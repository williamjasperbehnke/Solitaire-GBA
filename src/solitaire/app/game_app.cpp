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
    }

    game_app::game_app()
    {
        _selection.reset_tableau_pick_depth();
    }

    void game_app::update()
    {
        ++_runtime_frames;

        if(_phase == run_phase::awaiting_deal)
        {
            if(bn::keypad::start_pressed())
            {
                _begin_deal();
            }
        }
        else if(_phase == run_phase::dealing)
        {
            ++_deal_animation_frame;
            if(_deal_animation_frame >= deal_animation_total_frames)
            {
                _phase = run_phase::playing;
                _time_timer.restart();
                _elapsed_ticks = 0;
            }
        }
        else
        {
            if(! _game.has_won())
            {
                _elapsed_ticks = _time_timer.elapsed_ticks();
            }
            _update_input();
        }

        _renderer.render(_game, _selection, _elapsed_ticks, _moves_count, _phase == run_phase::awaiting_deal,
                         _phase == run_phase::dealing, _deal_animation_frame, _runtime_frames);
    }

    void game_app::_begin_deal()
    {
        _game.set_seed(_run_seed.next_auto_seed(_auto_seed_entropy()));
        _game.reset();
        _selection.reset_tableau_pick_depth();
        _time_timer.restart();
        _elapsed_ticks = 0;
        _moves_count = 0;
        _deal_animation_frame = 0;
        _phase = run_phase::dealing;
    }

    void game_app::_update_input()
    {
        const dpad_repeater::triggers dpad = _dpad_repeater.update();
        _selection.move_horizontal(dpad.left, dpad.right);

        const int selected_tableau_index = _selection.selected_tableau_index();
        const int face_up_count = selected_tableau_index >= 0 ? _game.tableau_face_up_size(selected_tableau_index) : 0;
        _selection.move_vertical(dpad.up, dpad.down, _game.has_held_card(), face_up_count);

        if(bn::keypad::start_pressed())
        {
            _game.cancel_held();
            _phase = run_phase::awaiting_deal;
            return;
        }

        if(bn::keypad::b_pressed())
        {
            _game.cancel_held();
            return;
        }

        if(! bn::keypad::a_pressed())
        {
            return;
        }

        _handle_primary_action();
    }

    void game_app::_reset_run_state()
    {
        _begin_deal();
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
            if(_game.try_place(selected))
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
            if(_game.draw_from_stock())
            {
                ++_moves_count;
            }
            return;
        }

        if(selected.kind == pile_kind::tableau)
        {
            if(_game.try_pick(selected, _selection.tableau_pick_depth_from_top()))
            {
                _selection.reset_tableau_pick_depth();
            }
            return;
        }

        (void) _game.try_pick(selected, 0);
    }

}
