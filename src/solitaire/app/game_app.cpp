#include "solitaire/app/game_app.h"

#include "bn_core.h"
#include "bn_keypad.h"

#include "solitaire/core/animation_timing.h"
#include "solitaire/core/table_layout.h"
#include "solitaire/render/held_panel_layout.h"
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
        constexpr int stock_draw_animation_frames = animation_timing::stock_draw_frames;
        constexpr int stock_recycle_animation_frames = animation_timing::stock_recycle_frames;
        constexpr int card_transfer_animation_frames = animation_timing::hold_release_frames;
        constexpr int cancel_max_animated_cards = 80;
        constexpr int waste_preview_count = render_constants::waste_preview_count;
        constexpr int waste_preview_step = render_constants::waste_preview_step;
        constexpr int tableau_face_down_step = render_constants::tableau_face_down_step;
        constexpr int held_cards_step = render_constants::held_cards_step;
        constexpr int held_cards_y = render_constants::held_cards_y;
        constexpr int selected_card_lift_x = render_constants::selected_card_lift_x;
        constexpr int selected_card_lift_y = render_constants::selected_card_lift_y;

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
        constexpr bool debug_start_tableau_spacing_example = false;

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
                    const int face_up_step = render_constants::tableau_face_up_step_for_count(face_up_count,
                                                                                               face_down_count);
                    out_x = table_layout::tableau_base_x + (selected.index * table_layout::pile_x_step);
                    out_y = table_layout::tableau_base_y + (face_down_count * tableau_face_down_step) +
                            ((face_up_count - 1) * face_up_step);
                    return true;
                }

                default:
                    return false;
            }
        }

        [[nodiscard]] bool tableau_card_position(const klondike_game& game, int tableau_index, int face_up_index,
                                                 int& out_x, int& out_y)
        {
            const int face_up_count = game.tableau_face_up_size(tableau_index);
            if(face_up_index < 0 || face_up_index >= face_up_count)
            {
                return false;
            }

            const int face_down_count = game.tableau_face_down_size(tableau_index);
            const int face_up_step = render_constants::tableau_face_up_step_for_count(face_up_count,
                                                                                       face_down_count);
            out_x = table_layout::tableau_base_x + (tableau_index * table_layout::pile_x_step);
            out_y = table_layout::tableau_base_y + (face_down_count * tableau_face_down_step) +
                    (face_up_index * face_up_step);
            return true;
        }

        [[nodiscard]] bool tableau_pick_source_position(const klondike_game& game, int tableau_index, int pick_depth,
                                                        int& out_x, int& out_y)
        {
            const int face_up_count = game.tableau_face_up_size(tableau_index);
            if(face_up_count <= 0)
            {
                return false;
            }

            int depth = pick_depth;
            if(depth < 0)
            {
                depth = 0;
            }
            else if(depth > face_up_count - 1)
            {
                depth = face_up_count - 1;
            }

            const int source_face_up_index = face_up_count - 1 - depth;
            return tableau_card_position(game, tableau_index, source_face_up_index, out_x, out_y);
        }

        [[nodiscard]] bool target_position_for_moved_cards(const klondike_game& game, const pile_ref& target_pile,
                                                           int moved_cards_count, int& out_x, int& out_y)
        {
            if(target_pile.kind == pile_kind::tableau && moved_cards_count > 1)
            {
                const int face_up_count = game.tableau_face_up_size(target_pile.index);
                const int base_up_index = face_up_count - moved_cards_count;
                return tableau_card_position(game, target_pile.index, base_up_index, out_x, out_y);
            }

            return top_card_position_for_selection(game, target_pile, out_x, out_y);
        }

        bool held_primary_position(const klondike_game& game, int& out_x, int& out_y)
        {
            const int held_count = game.held_cards_count();
            if(held_count <= 0)
            {
                return false;
            }

            out_x = held_panel_layout::first_card_x(held_count, held_cards_step);
            out_y = held_cards_y;
            return true;
        }

        void copy_held_cards(const klondike_game& game, bn::vector<card, 19>& out_cards)
        {
            out_cards.clear();
            const int held_count = game.held_cards_count();
            for(int index = 0; index < held_count; ++index)
            {
                out_cards.push_back(game.held_card(index));
            }
        }

        void apply_selected_lift_if_target_selected(const table_selection& selection, const pile_ref& target_pile,
                                                    int& x, int& y)
        {
            const pile_ref selected = selection.selected_pile();
            if(selected.kind == target_pile.kind && selected.index == target_pile.index)
            {
                x += selected_card_lift_x;
                y += selected_card_lift_y;
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
        const game_renderer::stock_draw_animation stock_draw_for_render = _renderer_stock_draw_animation();
        const game_renderer::stock_recycle_animation stock_recycle_for_render =
                _renderer_stock_recycle_animation();
        const game_renderer::card_transfer_animation transfer_for_render = _renderer_card_transfer_animation();
        const bn::string<48>* hint_text = _hint_overlay.text();
        const game_renderer::hint_highlight hint_cells = _hint_overlay.highlight();
        _renderer.render(_game, _selection, _elapsed_ticks, _moves_count, show_press_start_prompt,
                         show_deal_animation, _deal_animation_frame, show_cancel_animation,
                         _deal_animation_frame, show_victory_animation, _deal_animation_frame,
                         _stock_draw_animation.active ? &stock_draw_for_render : nullptr,
                         _stock_recycle_animation.active ? &stock_recycle_for_render : nullptr,
                         _card_transfer_animation.active ? &transfer_for_render : nullptr,
                         _runtime_frames, hint_text, hint_cells.has_move ? &hint_cells : nullptr);
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

        if(_update_active_playing_animations())
        {
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
        else if(debug_start_tableau_spacing_example)
        {
            _game.setup_debug_tableau_spacing_example();
            _selection.set_selected_pile({ pile_kind::tableau, 0 });
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
        _reset_all_playing_animations();
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

    int game_app::_tableau_destination_step(const pile_ref& pile) const
    {
        if(pile.kind != pile_kind::tableau)
        {
            return 0;
        }

        return render_constants::tableau_face_up_step_for_count(_game.tableau_face_up_size(pile.index),
                                                                _game.tableau_face_down_size(pile.index));
    }

    void game_app::_begin_transfer_to_target_pile(const bn::vector<card, 19>& cards, int source_x, int source_y,
                                                  const pile_ref& target_pile, int moved_cards_count)
    {
        int target_x = 0;
        int target_y = 0;
        if(! target_position_for_moved_cards(_game, target_pile, moved_cards_count, target_x, target_y))
        {
            return;
        }

        apply_selected_lift_if_target_selected(_selection, target_pile, target_x, target_y);
        _begin_card_transfer_animation(cards, source_x, source_y, target_x, target_y, held_cards_step, 0, 0,
                                       _tableau_destination_step(target_pile));
    }

    void game_app::_begin_transfer_to_held_from_source(const pile_ref& source_pile, int source_x, int source_y)
    {
        int target_x = 0;
        int target_y = 0;
        if(! held_primary_position(_game, target_x, target_y))
        {
            return;
        }

        bn::vector<card, 19> moving_cards;
        copy_held_cards(_game, moving_cards);
        int source_step_y = 0;
        if(source_pile.kind == pile_kind::tableau)
        {
            source_step_y = render_constants::tableau_face_up_step_for_count(
                    _game.tableau_face_up_size(source_pile.index) + moving_cards.size(),
                    _game.tableau_face_down_size(source_pile.index));
        }

        _begin_card_transfer_animation(moving_cards, source_x, source_y, target_x, target_y, 0, source_step_y,
                                       held_cards_step, 0);
    }

    void game_app::_cancel_held_with_return_animation()
    {
        if(! _game.has_held_card())
        {
            return;
        }

        const int held_cards_count = _game.held_cards_count();
        int source_x = 0;
        int source_y = 0;
        bn::vector<card, 19> moving_cards;
        pile_ref held_from = { pile_kind::stock, 0 };
        const bool can_animate_return =
                _capture_held_for_animation(held_from, source_x, source_y, moving_cards);

        _audio.on_cancel_pressed(true);
        _game.cancel_held();
        if(can_animate_return)
        {
            _begin_transfer_to_target_pile(moving_cards, source_x, source_y, held_from, held_cards_count);
        }
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
        _reset_all_playing_animations();
        _hint_overlay.clear();
        _audio.on_deal_started();
    }

    bool game_app::_capture_held_for_animation(pile_ref& held_from, int& source_x, int& source_y,
                                               bn::vector<card, 19>& cards) const
    {
        if(! _game.has_held_card())
        {
            return false;
        }

        held_from = _game.held_from();
        copy_held_cards(_game, cards);
        return held_primary_position(_game, source_x, source_y);
    }

    bool game_app::_update_active_playing_animations()
    {
        if(_stock_draw_animation.active)
        {
            ++_stock_draw_animation.frame;
            if(_stock_draw_animation.frame >= stock_draw_animation_frames)
            {
                _complete_stock_draw_animation();
                _reset_stock_draw_animation();
            }
            return true;
        }

        if(_stock_recycle_animation.active)
        {
            ++_stock_recycle_animation.frame;
            if(_stock_recycle_animation.frame >= stock_recycle_animation_frames)
            {
                _complete_stock_recycle_animation();
                _reset_stock_recycle_animation();
            }
            return true;
        }

        if(_card_transfer_animation.active)
        {
            ++_card_transfer_animation.frame;
            if(_card_transfer_animation.frame >= card_transfer_animation_frames)
            {
                _reset_card_transfer_animation();
            }
            return true;
        }

        return false;
    }

    void game_app::_reset_all_playing_animations()
    {
        _reset_stock_draw_animation();
        _reset_stock_recycle_animation();
        _reset_card_transfer_animation();
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
            const bool had_held_card = _game.has_held_card();
            if(had_held_card)
            {
                _cancel_held_with_return_animation();
            }
            else
            {
                _audio.on_cancel_pressed(false);
                _game.cancel_held();
            }
            return;
        }

        if(bn::keypad::r_pressed())
        {
            _handle_quick_send_to_foundation();
            return;
        }

        if(bn::keypad::l_pressed())
        {
            const bool had_held_card = _game.has_held_card();
            if(had_held_card)
            {
                _cancel_held_with_return_animation();
            }

            if(! _selection.is_stock_selected())
            {
                _selection.set_selected_pile({ pile_kind::stock, 0 });
                if(! had_held_card)
                {
                    _audio.on_selection_changed();
                }
            }

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
            pile_ref held_from = { pile_kind::stock, 0 };
            int source_x = 0;
            int source_y = 0;
            bn::vector<card, 19> moving_cards;
            const bool has_source = _capture_held_for_animation(held_from, source_x, source_y, moving_cards);
            const int held_cards_count = _game.held_cards_count();
            card previous_foundation_card;
            const bool had_previous_foundation_card =
                    selected.kind == pile_kind::foundation &&
                    _game.foundation_top(selected.index, previous_foundation_card);
            const bool success = _game.try_place(selected);
            _audio.on_place_attempt(success);
            if(success)
            {
                if(selected.kind == pile_kind::tableau)
                {
                    const int face_up_count = _game.tableau_face_up_size(selected.index);
                    int highlight_depth = held_cards_count - 1;
                    if(highlight_depth < 0)
                    {
                        highlight_depth = 0;
                    }
                    const int max_depth = face_up_count > 0 ? face_up_count - 1 : 0;
                    if(highlight_depth > max_depth)
                    {
                        highlight_depth = max_depth;
                    }
                    _selection.set_selected_pile(selected, highlight_depth);
                }

                if(selected.kind == pile_kind::foundation)
                {
                    if(has_source)
                    {
                        pile_ref destination = { pile_kind::foundation, selected.index };
                        int target_x = table_layout::foundation_base_x + (selected.index * table_layout::pile_x_step);
                        int target_y = table_layout::top_row_y;
                        apply_selected_lift_if_target_selected(_selection, destination, target_x, target_y);
                        _begin_card_transfer_animation(
                                moving_cards, source_x, source_y, target_x, target_y, held_cards_step, 0, 0, 0,
                                had_previous_foundation_card,
                                selected.index,
                                previous_foundation_card);
                    }
                }
                else
                {
                    const pile_ref target_pile = (selected.kind == held_from.kind && selected.index == held_from.index) ?
                                                         held_from :
                                                         selected;
                    if(has_source)
                    {
                        _begin_transfer_to_target_pile(moving_cards, source_x, source_y, target_pile,
                                                       held_cards_count);
                    }
                }

                if(selected.kind != held_from.kind || selected.index != held_from.index)
                {
                    ++_moves_count;
                }
            }
            return;
        }

        if(selected.kind == pile_kind::stock)
        {
            card moving_card;
            if(_game.stock_top(moving_card))
            {
                _begin_stock_draw_animation(moving_card);
                ++_moves_count;
                _audio.on_draw_from_stock(true);
            }
            else
            {
                if(_game.waste_size() > 0)
                {
                    _begin_stock_recycle_animation();
                    ++_moves_count;
                    _audio.on_draw_from_stock(true);
                }
                else
                {
                    _audio.on_draw_from_stock(false);
                }
            }
            return;
        }

        if(selected.kind == pile_kind::tableau)
        {
            int source_x = 0;
            int source_y = 0;
            const int pick_depth = _selection.tableau_pick_depth_from_top();
            const bool has_source = tableau_pick_source_position(_game, selected.index, pick_depth, source_x, source_y);
            const bool success = _game.try_pick(selected, pick_depth);
            _audio.on_pick_attempt(success);
            if(success)
            {
                _selection.reset_tableau_pick_depth();
                if(has_source)
                {
                    _begin_transfer_to_held_from_source(selected, source_x, source_y);
                }
            }
            return;
        }

        int source_x = 0;
        int source_y = 0;
        const bool has_source = top_card_position_for_selection(_game, selected, source_x, source_y);
        const bool success = _game.try_pick(selected, 0);
        _audio.on_pick_attempt(success);
        if(success)
        {
            if(has_source)
            {
                _begin_transfer_to_held_from_source(selected, source_x, source_y);
            }
        }
    }

    void game_app::_handle_quick_send_to_foundation()
    {
        int source_x = 0;
        int source_y = 0;
        bool picked_this_action = false;
        if(! _game.has_held_card())
        {
            const pile_ref selected = _selection.selected_pile();
            if(selected.kind == pile_kind::foundation)
            {
                return;
            }

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
            if(_game.held_from().kind == pile_kind::foundation)
            {
                return;
            }

            bn::vector<card, 19> moving_cards;
            pile_ref held_from = { pile_kind::stock, 0 };
            if(! _capture_held_for_animation(held_from, source_x, source_y, moving_cards))
            {
                _audio.on_place_attempt(false);
                return;
            }
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
            const pile_ref destination { pile_kind::foundation, placed_foundation_index };
            int target_x = table_layout::foundation_base_x + (placed_foundation_index * table_layout::pile_x_step);
            int target_y = table_layout::top_row_y;
            apply_selected_lift_if_target_selected(_selection, destination, target_x, target_y);
            _begin_card_transfer_animation(
                    moving_card, source_x, source_y, target_x, target_y, has_previous_card, placed_foundation_index,
                    previous_card);
            if(held_from.kind != destination.kind || held_from.index != destination.index)
            {
                ++_moves_count;
            }
        }
    }

    void game_app::_begin_stock_draw_animation(const card& moving_card)
    {
        _stock_draw_animation = {};
        _stock_draw_animation.active = true;
        _stock_draw_animation.moving_card = moving_card;
    }

    void game_app::_complete_stock_draw_animation()
    {
        if(! _game.draw_from_stock())
        {
            return;
        }

        klondike_game::waste_to_foundation_move move_result;
        if(_game.try_waste_to_foundation(move_result))
        {
            _begin_card_transfer_animation(
                    move_result.moved_card, table_layout::waste_x + ((waste_preview_count - 1) * waste_preview_step),
                    table_layout::top_row_y,
                    table_layout::foundation_base_x + (move_result.foundation_index * table_layout::pile_x_step),
                    table_layout::top_row_y, move_result.had_previous_card, move_result.foundation_index,
                    move_result.previous_card);
        }
    }

    void game_app::_reset_stock_draw_animation()
    {
        _stock_draw_animation = {};
    }

    game_renderer::stock_draw_animation game_app::_renderer_stock_draw_animation() const
    {
        game_renderer::stock_draw_animation output;
        output.frame = _stock_draw_animation.frame;
        output.moving_card = &_stock_draw_animation.moving_card;
        return output;
    }

    void game_app::_begin_stock_recycle_animation()
    {
        _stock_recycle_animation = {};
        _stock_recycle_animation.active = true;
    }

    void game_app::_complete_stock_recycle_animation()
    {
        if(! _game.draw_from_stock())
        {
            return;
        }
    }

    void game_app::_reset_stock_recycle_animation()
    {
        _stock_recycle_animation = {};
    }

    game_renderer::stock_recycle_animation game_app::_renderer_stock_recycle_animation() const
    {
        game_renderer::stock_recycle_animation output;
        output.frame = _stock_recycle_animation.frame;
        return output;
    }

    void game_app::_begin_card_transfer_animation(const card& moving_card, int source_x, int source_y, int target_x,
                                                  int target_y, bool show_previous_foundation_card,
                                                  int previous_foundation_index,
                                                  const card& previous_foundation_card)
    {
        bn::vector<card, 19> cards;
        cards.push_back(moving_card);
        _begin_card_transfer_animation(cards, source_x, source_y, target_x, target_y, 0, 0, 0, 0,
                                       show_previous_foundation_card, previous_foundation_index,
                                       previous_foundation_card);
    }

    void game_app::_begin_card_transfer_animation(const bn::vector<card, 19>& cards, int source_x, int source_y,
                                                  int target_x, int target_y, int source_stack_step_x,
                                                  int source_stack_step_y, int destination_stack_step_x,
                                                  int destination_stack_step_y, bool show_previous_foundation_card,
                                                  int previous_foundation_index,
                                                  const card& previous_foundation_card)
    {
        _card_transfer_animation = {};
        _card_transfer_animation.active = true;
        _card_transfer_animation.source_x = source_x;
        _card_transfer_animation.source_y = source_y;
        _card_transfer_animation.target_x = target_x;
        _card_transfer_animation.target_y = target_y;
        _card_transfer_animation.source_stack_step_x = source_stack_step_x;
        _card_transfer_animation.source_stack_step_y = source_stack_step_y;
        _card_transfer_animation.destination_stack_step_x = destination_stack_step_x;
        _card_transfer_animation.destination_stack_step_y = destination_stack_step_y;
        _card_transfer_animation.cards = cards;
        _card_transfer_animation.show_previous_foundation_card = show_previous_foundation_card;
        _card_transfer_animation.previous_foundation_index = previous_foundation_index;
        if(show_previous_foundation_card)
        {
            _card_transfer_animation.previous_foundation_card = previous_foundation_card;
        }
    }

    void game_app::_reset_card_transfer_animation()
    {
        _card_transfer_animation = {};
    }

    game_renderer::card_transfer_animation game_app::_renderer_card_transfer_animation() const
    {
        game_renderer::card_transfer_animation output;
        output.frame = _card_transfer_animation.frame;
        output.source_x = _card_transfer_animation.source_x;
        output.source_y = _card_transfer_animation.source_y;
        output.target_x = _card_transfer_animation.target_x;
        output.target_y = _card_transfer_animation.target_y;
        output.cards_count = _card_transfer_animation.cards.size();
        output.cards = output.cards_count > 0 ? _card_transfer_animation.cards.data() : nullptr;
        output.source_stack_step_x = _card_transfer_animation.source_stack_step_x;
        output.source_stack_step_y = _card_transfer_animation.source_stack_step_y;
        output.destination_stack_step_x = _card_transfer_animation.destination_stack_step_x;
        output.destination_stack_step_y = _card_transfer_animation.destination_stack_step_y;
        output.show_previous_foundation_card = _card_transfer_animation.show_previous_foundation_card;
        output.previous_foundation_index = _card_transfer_animation.previous_foundation_index;
        if(_card_transfer_animation.show_previous_foundation_card)
        {
            output.previous_foundation_card = &_card_transfer_animation.previous_foundation_card;
        }
        return output;
    }

}
