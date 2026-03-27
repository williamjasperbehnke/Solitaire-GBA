#include "solitaire/input/dpad_repeater.h"

#include "bn_keypad.h"

namespace solitaire
{
    namespace
    {
        constexpr int initial_delay = 16;
        constexpr int repeat_interval = 4;
    }

    dpad_repeater::triggers dpad_repeater::update()
    {
        triggers output;
        output.left = _update_button(bn::keypad::left_pressed(), bn::keypad::left_held(), _left_counter);
        output.right = _update_button(bn::keypad::right_pressed(), bn::keypad::right_held(), _right_counter);
        output.up = _update_button(bn::keypad::up_pressed(), bn::keypad::up_held(), _up_counter);
        output.down = _update_button(bn::keypad::down_pressed(), bn::keypad::down_held(), _down_counter);
        return output;
    }

    bool dpad_repeater::_update_button(bool pressed, bool held, int& counter)
    {
        if(pressed)
        {
            counter = initial_delay;
            return true;
        }

        if(held)
        {
            if(counter > 0)
            {
                --counter;
                return false;
            }

            counter = repeat_interval;
            return true;
        }

        counter = 0;
        return false;
    }
}
