#ifndef SOLITAIRE_RENDER_CONSTANTS_H
#define SOLITAIRE_RENDER_CONSTANTS_H

namespace solitaire::render_constants
{
    constexpr int waste_preview_count = 3;
    constexpr int waste_preview_step = 8;
    constexpr int tableau_face_down_step = 4;

    [[nodiscard]] constexpr int tableau_face_up_step_for_count(int face_up_count)
    {
        if(face_up_count <= 6)
        {
            return 7;
        }
        if(face_up_count <= 10)
        {
            return 6;
        }
        if(face_up_count <= 14)
        {
            return 5;
        }
        if(face_up_count <= 18)
        {
            return 4;
        }
        return 3;
    }
}

#endif
