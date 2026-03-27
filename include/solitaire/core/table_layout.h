#ifndef SOLITAIRE_TABLE_LAYOUT_H
#define SOLITAIRE_TABLE_LAYOUT_H

namespace solitaire::table_layout
{
    constexpr int stock_selection_index = 0;
    constexpr int waste_selection_index = 1;
    constexpr int first_foundation_selection_index = 2;
    constexpr int last_foundation_selection_index = 5;
    constexpr int first_tableau_selection_index = 6;
    constexpr int last_selection_index = 12;

    constexpr int stock_x = -96;
    constexpr int waste_x = -64;
    constexpr int foundation_base_x = 0;
    constexpr int tableau_base_x = -96;
    constexpr int pile_x_step = 32;

    constexpr int top_row_y = -42;
    constexpr int tableau_base_y = -2;
}

#endif
