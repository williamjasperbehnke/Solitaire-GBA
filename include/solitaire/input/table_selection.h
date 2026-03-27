#ifndef SOLITAIRE_TABLE_SELECTION_H
#define SOLITAIRE_TABLE_SELECTION_H

#include "bn_string.h"

#include "solitaire/core/pile_ref.h"

namespace solitaire
{

    class table_selection
    {
    public:
        struct highlight_state
        {
            int x = 0;
            int y = 0;
            bool use_waste_style = false;
        };

        [[nodiscard]] pile_ref selected_pile() const;
        [[nodiscard]] bn::string<8> label() const;
        [[nodiscard]] highlight_state highlight() const;

        [[nodiscard]] int selected_index() const;
        [[nodiscard]] int tableau_pick_depth_from_top() const;

        [[nodiscard]] bool is_stock_selected() const;
        [[nodiscard]] bool is_waste_selected() const;
        [[nodiscard]] int selected_foundation_index() const;
        [[nodiscard]] int selected_tableau_index() const;

        void move_horizontal(bool left_trigger, bool right_trigger);
        void move_vertical(bool up_trigger, bool down_trigger, bool has_held_card, int selected_tableau_face_up_count);
        void reset_tableau_pick_depth();
        void set_selected_pile(const pile_ref& pile, int tableau_pick_depth_from_top = 0);

    private:
        [[nodiscard]] static int _pile_x_for_index(int selected_index);
        [[nodiscard]] static int _nearest_top_row_index_for_x(int x);
        [[nodiscard]] static int _nearest_tableau_index_for_x(int x);

        int _selected_index = 0;
        int _tableau_pick_depth_from_top = 0;
    };

}

#endif
