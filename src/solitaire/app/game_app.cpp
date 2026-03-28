#include "solitaire/app/game_app.h"

#include "bn_core.h"
#include "bn_keypad.h"

#include "solitaire/core/animation_timing.h"
#include "solitaire/core/table_layout.h"
#include "solitaire/render/render_constants.h"

namespace solitaire
{
    namespace
    {
        constexpr int deal_animation_frames_per_card = animation_timing::deal_frames_per_card;
        constexpr int deal_animation_cards = animation_timing::deal_cards;
        constexpr int deal_animation_total_frames = deal_animation_frames_per_card * deal_animation_cards;
        constexpr int victory_animation_total_frames = 180;
        constexpr int cancel_animation_stagger_frames = animation_timing::cancel_stagger_frames;
        constexpr int cancel_animation_travel_frames = animation_timing::cancel_travel_frames;
        constexpr int foundation_move_animation_frames = animation_timing::foundation_move_frames;
        constexpr int cancel_max_animated_cards = 80;
        constexpr int waste_preview_count = render_constants::waste_preview_count;
        constexpr int waste_preview_step = render_constants::waste_preview_step;
        constexpr int tableau_face_down_step = render_constants::tableau_face_down_step;

        // Disabled by default to keep deal start instant.
        constexpr bool enable_winnability_filter = false;

        constexpr int strict_easy_win_depth_limit = 28;
        constexpr int strict_hard_search_depth_limit = 160;
        constexpr int strict_easy_search_node_budget = 28;
        constexpr int strict_hard_search_node_budget = 160;
        constexpr int max_strict_deal_attempts = 32;
        constexpr int fallback_winnable_depth_limit = 120;
        constexpr int max_total_deal_attempts = 80;

        // Developer preview mode to quickly verify win flow and animation.
        constexpr bool debug_start_one_move_from_win = false;

        [[nodiscard]] bool top_card_position_for_selection(const klondike_game& game, const pile_ref& selected,
                                                           int& out_x, int& out_y)
        {
            switch(selected.kind)
            {
                case pile_kind::stock:
                    return false;

                case pile_kind::waste:
                    if(game.waste_size() <= 0)
                    {
                        return false;
                    }
                    out_x = table_layout::waste_x + ((waste_preview_count - 1) * waste_preview_step);
                    out_y = table_layout::top_row_y;
                    return true;

                case pile_kind::foundation:
                    if(game.foundation_size(selected.index) <= 0)
                    {
                        return false;
                    }
                    out_x = table_layout::foundation_base_x + (selected.index * table_layout::pile_x_step);
                    out_y = table_layout::top_row_y;
                    return true;

                case pile_kind::tableau:
                {
                    const int face_up_count = game.tableau_face_up_size(selected.index);
                    if(face_up_count <= 0)
                    {
                        return false;
                    }
                    const int face_down_count = game.tableau_face_down_size(selected.index);
                    const int face_up_step = render_constants::tableau_face_up_step_for_count(face_up_count);
                    out_x = table_layout::tableau_base_x + (selected.index * table_layout::pile_x_step);
                    out_y = table_layout::tableau_base_y + (face_down_count * tableau_face_down_step) +
                            ((face_up_count - 1) * face_up_step);
                    return true;
                }

                default:
                    return false;
            }
        }
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
        _update_phase();
        _render_frame();
    }

    void game_app::_update_phase()
    {
        switch(_phase)
        {
            case run_phase::awaiting_deal:
                _update_awaiting_deal_phase();
                break;

            case run_phase::dealing:
                _update_dealing_phase();
                break;

            case run_phase::playing:
                _update_playing_phase();
                break;

            case run_phase::canceling:
                _update_canceling_phase();
                break;

            case run_phase::victory:
                _update_victory_phase();
                break;

            default:
                break;
        }
    }

    void game_app::_render_frame()
    {
        const bool show_press_start_prompt = _phase == run_phase::awaiting_deal;
        const bool show_deal_animation = _phase == run_phase::dealing;
        const bool show_cancel_animation = _phase == run_phase::canceling;
        const bool show_victory_animation = _phase == run_phase::victory;
        const game_renderer::foundation_move_animation foundation_move_for_render =
                _renderer_foundation_move_animation();
        const bn::string<48>* hint_text = _hint_overlay.text();
        const game_renderer::hint_highlight hint_cells = _hint_overlay.highlight();
        _renderer.render(_game, _selection, _elapsed_ticks, _moves_count, show_press_start_prompt,
                         show_deal_animation, _deal_animation_frame, show_cancel_animation,
                         _deal_animation_frame, show_victory_animation, _deal_animation_frame,
                         _foundation_move_animation.active ? &foundation_move_for_render : nullptr,
                         _runtime_frames, hint_text,
                         hint_cells.has_move ? &hint_cells : nullptr);
    }

    void game_app::_update_awaiting_deal_phase()
    {
        if(bn::keypad::start_pressed())
        {
            _begin_deal();
        }
    }

    void game_app::_update_dealing_phase()
    {
        ++_deal_animation_frame;
        _audio.on_deal_animation_frame(_deal_animation_frame, deal_animation_frames_per_card, deal_animation_cards);

        if(_deal_animation_frame >= deal_animation_total_frames)
        {
            _phase = run_phase::playing;
            _time_timer.restart();
            _elapsed_ticks = 0;
        }
    }

    void game_app::_update_playing_phase()
    {
        _elapsed_ticks = _time_timer.elapsed_ticks();

        if(_foundation_move_animation.active)
        {
            ++_foundation_move_animation.frame;
            if(_foundation_move_animation.frame >= foundation_move_animation_frames)
            {
                _reset_foundation_move_animation();
            }
            return;
        }

        if(_game.has_won())
        {
            _audio.on_win_state(true);
            _phase = run_phase::victory;
            _deal_animation_frame = 0;
            _hint_overlay.clear();
            return;
        }

        _update_input();
        _audio.on_win_state(false);
    }

    void game_app::_update_canceling_phase()
    {
        ++_deal_animation_frame;
        _audio.on_deal_animation_frame(_deal_animation_frame, cancel_animation_stagger_frames,
                                       _cancel_animation_cards);
        if(_deal_animation_frame >= _cancel_animation_total_frames)
        {
            _phase = run_phase::awaiting_deal;
            _deal_animation_frame = 0;
            _cancel_animation_cards = 0;
            _cancel_animation_total_frames = 0;
        }
    }

    void game_app::_update_victory_phase()
    {
        ++_deal_animation_frame;
        if(_deal_animation_frame >= victory_animation_total_frames)
        {
            _phase = run_phase::awaiting_deal;
            _deal_animation_frame = 0;
        }
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

        if(debug_start_one_move_from_win)
        {
            _game.setup_debug_one_move_to_win();
            _selection.set_selected_pile({ pile_kind::waste, 0 });
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
        _reset_foundation_move_animation();
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

    int game_app::_cancel_animation_cards_count() const
    {
        int count = 0;

        if(_game.stock_size() > 0)
        {
            ++count;
        }

        int waste_preview = _game.waste_size();
        if(waste_preview > waste_preview_count)
        {
            waste_preview = waste_preview_count;
        }
        count += waste_preview;

        for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
        {
            if(_game.foundation_size(foundation_index) > 0)
            {
                ++count;
            }
        }

        for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
        {
            count += _game.tableau_face_down_size(tableau_index);
            count += _game.tableau_face_up_size(tableau_index);
        }

        if(count > cancel_max_animated_cards)
        {
            count = cancel_max_animated_cards;
        }

        return count;
    }

    void game_app::_begin_cancel_sequence()
    {
        _game.cancel_held();
        _phase = run_phase::canceling;
        _deal_animation_frame = 0;
        _cancel_animation_cards = _cancel_animation_cards_count();
        _cancel_animation_total_frames =
                (_cancel_animation_cards * cancel_animation_stagger_frames) +
                cancel_animation_travel_frames;
        _reset_foundation_move_animation();
        _hint_overlay.clear();
        _audio.on_deal_started();
    }

    void game_app::_update_input()
    {
        const int previous_selected_index = _selection.selected_index();
        const int previous_pick_depth = _selection.tableau_pick_depth_from_top();

        const dpad_repeater::triggers dpad = _dpad_repeater.update();
        const bool non_select_input = dpad.left || dpad.right || dpad.up || dpad.down ||
                                      bn::keypad::a_pressed() || bn::keypad::b_pressed() ||
                                      bn::keypad::start_pressed() || bn::keypad::l_pressed() ||
                                      bn::keypad::r_pressed();
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
            _begin_cancel_sequence();
            return;
        }

        if(bn::keypad::b_pressed())
        {
            _audio.on_cancel_pressed(_game.has_held_card());
            _game.cancel_held();
            return;
        }

        if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
        {
            _handle_quick_send_to_foundation();
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
            const bool stock_had_cards = _game.stock_size() > 0;
            const bool success = _game.draw_from_stock();
            _audio.on_draw_from_stock(success);
            if(success)
            {
                ++_moves_count;
                if(stock_had_cards)
                {
                    klondike_game::waste_to_foundation_move move_result;
                    if(_game.try_waste_to_foundation(move_result))
                    {
                        _begin_foundation_move_animation(
                                move_result.moved_card,
                                table_layout::waste_x + ((waste_preview_count - 1) * waste_preview_step),
                                table_layout::top_row_y, move_result.foundation_index,
                                move_result.had_previous_card, move_result.previous_card);
                    }
                }
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

    void game_app::_handle_quick_send_to_foundation()
    {
        int source_x = 0;
        int source_y = 0;
        bool picked_this_action = false;
        if(! _game.has_held_card())
        {
            const pile_ref selected = _selection.selected_pile();
            if(! top_card_position_for_selection(_game, selected, source_x, source_y))
            {
                _audio.on_place_attempt(false);
                return;
            }

            if(! _game.try_pick(selected, 0))
            {
                _audio.on_place_attempt(false);
                return;
            }

            if(selected.kind == pile_kind::tableau)
            {
                _selection.reset_tableau_pick_depth();
            }
            picked_this_action = true;
        }
        else
        {
            const table_selection::highlight_state highlight = _selection.highlight();
            source_x = highlight.x;
            source_y = highlight.y;
        }

        if(_game.held_cards_count() != 1)
        {
            if(picked_this_action)
            {
                _game.cancel_held();
            }
            _audio.on_place_attempt(false);
            return;
        }

        const card moving_card = _game.held_card(0);
        const pile_ref held_from = _game.held_from();
        bool placed = false;
        int placed_foundation_index = -1;
        bool has_previous_card = false;
        card previous_card;
        for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
        {
            card candidate_previous_card;
            const bool candidate_has_previous = _game.foundation_top(foundation_index, candidate_previous_card);
            if(_game.try_place({ pile_kind::foundation, foundation_index }))
            {
                placed = true;
                placed_foundation_index = foundation_index;
                has_previous_card = candidate_has_previous;
                previous_card = candidate_previous_card;
                break;
            }
        }

        if(! placed && picked_this_action)
        {
            _game.cancel_held();
        }

        _audio.on_place_attempt(placed);
        if(placed)
        {
            _begin_foundation_move_animation(moving_card, source_x, source_y, placed_foundation_index, has_previous_card,
                                             previous_card);
            const pile_ref destination { pile_kind::foundation, placed_foundation_index };
            if(held_from.kind != destination.kind || held_from.index != destination.index)
            {
                ++_moves_count;
            }
        }
    }

    void game_app::_begin_foundation_move_animation(const card& moving_card, int source_x, int source_y,
                                                    int destination_foundation_index, bool has_previous_card,
                                                    const card& previous_card)
    {
        _foundation_move_animation = {};
        _foundation_move_animation.active = true;
        _foundation_move_animation.source_x = source_x;
        _foundation_move_animation.source_y = source_y;
        _foundation_move_animation.destination_foundation_index = destination_foundation_index;
        _foundation_move_animation.moving_card = moving_card;
        _foundation_move_animation.has_previous_card = has_previous_card;
        if(has_previous_card)
        {
            _foundation_move_animation.previous_card = previous_card;
        }
    }

    void game_app::_reset_foundation_move_animation()
    {
        _foundation_move_animation = {};
    }

    game_renderer::foundation_move_animation game_app::_renderer_foundation_move_animation() const
    {
        game_renderer::foundation_move_animation output;
        output.frame = _foundation_move_animation.frame;
        output.source_x = _foundation_move_animation.source_x;
        output.source_y = _foundation_move_animation.source_y;
        output.destination_index = _foundation_move_animation.destination_foundation_index;
        output.moving_card = &_foundation_move_animation.moving_card;
        if(_foundation_move_animation.has_previous_card)
        {
            output.previous_destination_card = &_foundation_move_animation.previous_card;
        }
        return output;
    }

}
