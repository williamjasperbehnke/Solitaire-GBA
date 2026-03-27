#include "solitaire/app/game_app.h"

#include "bn_core.h"
#include "bn_keypad.h"

namespace solitaire
{

    game_app::game_app()
    {
        const unsigned initial_entropy = unsigned(bn::core::current_cpu_ticks()) ^
                                         (unsigned(_time_timer.elapsed_ticks()) << 1) ^
                                         0xA5A5A5A5u;
        _game.add_entropy(initial_entropy);
        _reset_run_state();
    }

    void game_app::update()
    {
        if(! _game.has_won())
        {
            _elapsed_ticks = _time_timer.elapsed_ticks();
        }

        const unsigned runtime_entropy = (_entropy_counter * 0x9E3779B9u) ^
                                         unsigned(bn::core::current_cpu_ticks()) ^
                                         (unsigned(_elapsed_ticks) << 11) ^
                                         (bn::keypad::any_held() ? 0xD00DFEEDu : 0u);
        _game.add_entropy(runtime_entropy);
        ++_entropy_counter;

        _update_input();
        _renderer.render(_game, _selection, _elapsed_ticks, _moves_count);
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
            _reset_run_state();
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
        _game.reset();
        _selection.reset_tableau_pick_depth();
        _time_timer.restart();
        _elapsed_ticks = 0;
        _moves_count = 0;
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
