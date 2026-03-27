#ifndef SOLITAIRE_DPAD_REPEATER_H
#define SOLITAIRE_DPAD_REPEATER_H

namespace solitaire
{

    class dpad_repeater
    {
    public:
        struct triggers
        {
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
        };

        [[nodiscard]] triggers update();

    private:
        [[nodiscard]] static bool _update_button(bool pressed, bool held, int& counter);

        int _left_counter = 0;
        int _right_counter = 0;
        int _up_counter = 0;
        int _down_counter = 0;
    };

}

#endif
