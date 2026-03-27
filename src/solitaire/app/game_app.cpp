#include "solitaire/app/game_app.h"

#include "bn_core.h"
#include "bn_keypad.h"

namespace solitaire
{
    namespace
    {
        constexpr int deal_animation_frames_per_card = 3;
        constexpr int deal_animation_cards = 28;
        constexpr int deal_animation_total_frames = deal_animation_frames_per_card * deal_animation_cards;

        // Disabled by default to keep deal start instant.
        constexpr bool enable_winnability_filter = false;

        constexpr int strict_easy_win_depth_limit = 28;
        constexpr int strict_hard_search_depth_limit = 160;
        constexpr int strict_easy_search_node_budget = 28;
        constexpr int strict_hard_search_node_budget = 160;
        constexpr int max_strict_deal_attempts = 32;
        constexpr int fallback_winnable_depth_limit = 120;
        constexpr int max_total_deal_attempts = 80;
    }

    game_app::game_app()
    {
        _selection.reset_tableau_pick_depth();
    }

    void game_app::update()
    {
        ++_runtime_frames;
        _audio.update();
        _hint_overlay.update();

        switch(_phase)
        {
            case run_phase::awaiting_deal:
            {
                if(bn::keypad::start_pressed())
                {
                    _begin_deal();
                }
                break;
            }

            case run_phase::dealing:
            {
                ++_deal_animation_frame;
                _audio.on_deal_animation_frame(_deal_animation_frame, deal_animation_frames_per_card, deal_animation_cards);

                if(_deal_animation_frame >= deal_animation_total_frames)
                {
                    _phase = run_phase::playing;
                    _time_timer.restart();
                    _elapsed_ticks = 0;
                }
                break;
            }

            case run_phase::playing:
            {
                if(! _game.has_won())
                {
                    _elapsed_ticks = _time_timer.elapsed_ticks();
                }
                _update_input();
                _audio.on_win_state(_game.has_won());
                break;
            }

            default:
                break;
        }

        const bool show_press_start_prompt = _phase == run_phase::awaiting_deal;
        const bool show_deal_animation = _phase == run_phase::dealing;
        const bn::string<48>* hint_text = _hint_overlay.text();
        const game_renderer::hint_highlight hint_cells = _hint_overlay.highlight();
        _renderer.render(_game, _selection, _elapsed_ticks, _moves_count, show_press_start_prompt,
                         show_deal_animation, _deal_animation_frame, _runtime_frames, hint_text,
                         hint_cells.has_move ? &hint_cells : nullptr);
    }

    void game_app::_begin_deal()
    {
        if(enable_winnability_filter)
        {
            _deal_with_filter();
        }
        else
        {
            _deal_once();
        }

        _reset_round_state_for_new_deal();
    }

    void game_app::_deal_once()
    {
        _game.set_seed(_run_seed.next_auto_seed(_auto_seed_entropy()));
        _game.reset();
    }

    void game_app::_deal_with_filter()
    {
        int attempts = 0;
        while(attempts < max_strict_deal_attempts)
        {
            _deal_once();
            ++attempts;

            if(_game.classify_deal_with_search(strict_easy_win_depth_limit, strict_hard_search_depth_limit,
                                               strict_easy_search_node_budget,
                                               strict_hard_search_node_budget) ==
               deal_quality::acceptable)
            {
                return;
            }
        }

        while(attempts < max_total_deal_attempts &&
              ! _game.is_winnable_with_search(fallback_winnable_depth_limit))
        {
            _deal_once();
            ++attempts;
        }
    }

    void game_app::_reset_round_state_for_new_deal()
    {
        _selection.reset_tableau_pick_depth();
        _time_timer.restart();
        _elapsed_ticks = 0;
        _moves_count = 0;
        _deal_animation_frame = 0;
        _phase = run_phase::dealing;
        _hint_overlay.clear();
        _audio.on_deal_started();
    }

    void game_app::_show_hint()
    {
        const klondike_hint_service::hint_result hint = _hint_service.build_hint(_game);
        _hint_overlay.show(hint, 180);
        _hint_overlay.apply_selection(_selection);
        _audio.on_selection_changed();
    }

    void game_app::_update_input()
    {
        const int previous_selected_index = _selection.selected_index();
        const int previous_pick_depth = _selection.tableau_pick_depth_from_top();

        const dpad_repeater::triggers dpad = _dpad_repeater.update();
        const bool non_select_input = dpad.left || dpad.right || dpad.up || dpad.down ||
                                      bn::keypad::a_pressed() || bn::keypad::b_pressed() ||
                                      bn::keypad::start_pressed();
        if(non_select_input)
        {
            _hint_overlay.clear();
        }

        _selection.move_horizontal(dpad.left, dpad.right);

        const int selected_tableau_index = _selection.selected_tableau_index();
        const int face_up_count = selected_tableau_index >= 0 ? _game.tableau_face_up_size(selected_tableau_index) : 0;
        _selection.move_vertical(dpad.up, dpad.down, _game.has_held_card(), face_up_count);
        if(_selection.selected_index() != previous_selected_index ||
           _selection.tableau_pick_depth_from_top() != previous_pick_depth)
        {
            _audio.on_selection_changed();
        }

        if(bn::keypad::start_pressed())
        {
            _game.cancel_held();
            _phase = run_phase::awaiting_deal;
            _audio.on_return_to_prompt();
            return;
        }

        if(bn::keypad::b_pressed())
        {
            _audio.on_cancel_pressed(_game.has_held_card());
            _game.cancel_held();
            return;
        }

        if(! bn::keypad::a_pressed())
        {
            if(bn::keypad::select_pressed())
            {
                _show_hint();
            }
            return;
        }

        _handle_primary_action();
    }

    unsigned game_app::_auto_seed_entropy() const
    {
        // Keeps blockudoku's cycle-based source and mixes in runtime progression.
        return unsigned(bn::core::current_cpu_ticks()) ^
               (unsigned(_entropy_timer.elapsed_ticks()) << 8) ^
               (_runtime_frames * 0x9E3779B9u) ^
               (bn::keypad::any_held() ? 0xA511E9B3u : 0u);
    }

    void game_app::_handle_primary_action()
    {
        const pile_ref selected = _selection.selected_pile();

        if(_game.has_held_card())
        {
            const pile_ref held_from = _game.held_from();
            const bool success = _game.try_place(selected);
            _audio.on_place_attempt(success);
            if(success)
            {
                if(selected.kind != held_from.kind || selected.index != held_from.index)
                {
                    ++_moves_count;
                }
            }
            return;
        }

        if(selected.kind == pile_kind::stock)
        {
            const bool success = _game.draw_from_stock();
            _audio.on_draw_from_stock(success);
            if(success)
            {
                ++_moves_count;
            }
            return;
        }

        if(selected.kind == pile_kind::tableau)
        {
            const bool success = _game.try_pick(selected, _selection.tableau_pick_depth_from_top());
            _audio.on_pick_attempt(success);
            if(success)
            {
                _selection.reset_tableau_pick_depth();
            }
            return;
        }

        _audio.on_pick_attempt(_game.try_pick(selected, 0));
    }

}
