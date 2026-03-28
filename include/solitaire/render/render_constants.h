#ifndef SOLITAIRE_RENDER_CONSTANTS_H
#define SOLITAIRE_RENDER_CONSTANTS_H

namespace solitaire::render_constants
{
    constexpr int waste_preview_count = 3;
    constexpr int waste_preview_step = 8;
    constexpr int tableau_face_down_step = 4;
    constexpr int held_cards_step = 8;
    constexpr int held_cards_y = 52;
    constexpr int selected_card_lift_x = 4;
    constexpr int selected_card_lift_y = -4;

    [[nodiscard]] constexpr int tableau_face_up_step_for_count(int face_up_count, int face_down_count = 0)
    {
        if(face_up_count <= 0)
        {
            return 1;
        }

        constexpr int tableau_height_limit = 72;
        constexpr int max_face_up_step = 9;
        const int available_pixels = tableau_height_limit - (face_down_count * tableau_face_down_step);
        if(available_pixels <= 0)
        {
            return 1;
        }

        const int step = available_pixels / face_up_count;
        if(step <= 0)
        {
            return 1;
        }
        if(step > max_face_up_step)
        {
            return max_face_up_step;
        }
        return step;
    }
}

#endif
