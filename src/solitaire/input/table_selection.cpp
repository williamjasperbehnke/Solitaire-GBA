#include "solitaire/input/table_selection.h"

#include "solitaire/core/table_layout.h"

namespace solitaire
{
    namespace
    {
        [[nodiscard]] int abs_distance(int left, int right)
        {
            const int diff = left - right;
            return diff < 0 ? -diff : diff;
        }
    }

    pile_ref table_selection::selected_pile() const
    {
        if(_selected_index == table_layout::stock_selection_index)
        {
            return pile_ref { pile_kind::stock, 0 };
        }

        if(_selected_index == table_layout::waste_selection_index)
        {
            return pile_ref { pile_kind::waste, 0 };
        }

        if(_selected_index >= table_layout::first_foundation_selection_index &&
           _selected_index <= table_layout::last_foundation_selection_index)
        {
            return pile_ref { pile_kind::foundation, _selected_index - table_layout::first_foundation_selection_index };
        }

        return pile_ref { pile_kind::tableau, _selected_index - table_layout::first_tableau_selection_index };
    }

    bn::string<8> table_selection::label() const
    {
        bn::string<8> output;
        if(is_stock_selected())
        {
            output = "STK";
        }
        else if(is_waste_selected())
        {
            output = "WST";
        }
        else if(int foundation_index = selected_foundation_index(); foundation_index >= 0)
        {
            output = "F";
            output += bn::to_string<1>(foundation_index + 1);
        }
        else
        {
            output = "T";
            output += bn::to_string<1>(selected_tableau_index() + 1);
        }

        return output;
    }

    table_selection::highlight_state table_selection::highlight() const
    {
        highlight_state state;

        if(is_stock_selected())
        {
            state.x = table_layout::stock_x;
            state.y = table_layout::top_row_y;
        }
        else if(is_waste_selected())
        {
            state.x = table_layout::waste_x;
            state.y = table_layout::top_row_y;
        }
        else if(int foundation_index = selected_foundation_index(); foundation_index >= 0)
        {
            state.x = table_layout::foundation_base_x + (foundation_index * table_layout::pile_x_step);
            state.y = table_layout::top_row_y;
        }
        else
        {
            state.x = table_layout::tableau_base_x + (selected_tableau_index() * table_layout::pile_x_step);
            state.y = table_layout::tableau_base_y;
        }

        state.use_waste_style = is_waste_selected();
        return state;
    }

    int table_selection::selected_index() const
    {
        return _selected_index;
    }

    int table_selection::tableau_pick_depth_from_top() const
    {
        return _tableau_pick_depth_from_top;
    }

    bool table_selection::is_stock_selected() const
    {
        return _selected_index == table_layout::stock_selection_index;
    }

    bool table_selection::is_waste_selected() const
    {
        return _selected_index == table_layout::waste_selection_index;
    }

    int table_selection::selected_foundation_index() const
    {
        if(_selected_index >= table_layout::first_foundation_selection_index &&
           _selected_index <= table_layout::last_foundation_selection_index)
        {
            return _selected_index - table_layout::first_foundation_selection_index;
        }

        return -1;
    }

    int table_selection::selected_tableau_index() const
    {
        if(_selected_index >= table_layout::first_tableau_selection_index)
        {
            return _selected_index - table_layout::first_tableau_selection_index;
        }

        return -1;
    }

    void table_selection::move_horizontal(bool left_trigger, bool right_trigger)
    {
        if(left_trigger)
        {
            --_selected_index;
            if(_selected_index < 0)
            {
                _selected_index = table_layout::last_selection_index;
            }
            _tableau_pick_depth_from_top = 0;
        }

        if(right_trigger)
        {
            ++_selected_index;
            if(_selected_index > table_layout::last_selection_index)
            {
                _selected_index = 0;
            }
            _tableau_pick_depth_from_top = 0;
        }
    }

    void table_selection::move_vertical(bool up_trigger, bool down_trigger, bool has_held_card,
                                        int selected_tableau_face_up_count)
    {
        if(int tableau_index = selected_tableau_index(); tableau_index >= 0)
        {
            if(has_held_card)
            {
                _tableau_pick_depth_from_top = 0;

                if(up_trigger)
                {
                    const int x = _pile_x_for_index(_selected_index);
                    _selected_index = _nearest_top_row_index_for_x(x);
                }

                return;
            }

            const int max_depth = selected_tableau_face_up_count > 0 ? selected_tableau_face_up_count - 1 : 0;
            if(_tableau_pick_depth_from_top > max_depth)
            {
                _tableau_pick_depth_from_top = max_depth;
            }

            if(! has_held_card && up_trigger && _tableau_pick_depth_from_top < max_depth)
            {
                ++_tableau_pick_depth_from_top;
            }
            else if(up_trigger && _tableau_pick_depth_from_top >= max_depth)
            {
                const int x = _pile_x_for_index(_selected_index);
                _selected_index = _nearest_top_row_index_for_x(x);
                _tableau_pick_depth_from_top = 0;
            }

            if(! has_held_card && down_trigger && _tableau_pick_depth_from_top > 0)
            {
                --_tableau_pick_depth_from_top;
            }
        }
        else if(down_trigger)
        {
            const int x = _pile_x_for_index(_selected_index);
            _selected_index = _nearest_tableau_index_for_x(x);
            _tableau_pick_depth_from_top = 0;
        }
    }

    void table_selection::reset_tableau_pick_depth()
    {
        _tableau_pick_depth_from_top = 0;
    }

    void table_selection::set_selected_pile(const pile_ref& pile, int tableau_pick_depth_from_top)
    {
        switch(pile.kind)
        {
            case pile_kind::stock:
                _selected_index = table_layout::stock_selection_index;
                _tableau_pick_depth_from_top = 0;
                break;

            case pile_kind::waste:
                _selected_index = table_layout::waste_selection_index;
                _tableau_pick_depth_from_top = 0;
                break;

            case pile_kind::foundation:
                _selected_index = table_layout::first_foundation_selection_index + pile.index;
                _tableau_pick_depth_from_top = 0;
                break;

            case pile_kind::tableau:
                _selected_index = table_layout::first_tableau_selection_index + pile.index;
                _tableau_pick_depth_from_top = tableau_pick_depth_from_top < 0 ? 0 : tableau_pick_depth_from_top;
                break;
            default:
                break;
        }
    }

    int table_selection::_pile_x_for_index(int selected_index)
    {
        if(selected_index == table_layout::stock_selection_index)
        {
            return table_layout::stock_x;
        }
        if(selected_index == table_layout::waste_selection_index)
        {
            return table_layout::waste_x;
        }
        if(selected_index >= table_layout::first_foundation_selection_index &&
           selected_index <= table_layout::last_foundation_selection_index)
        {
            return table_layout::foundation_base_x +
                   ((selected_index - table_layout::first_foundation_selection_index) * table_layout::pile_x_step);
        }

        return table_layout::tableau_base_x +
               ((selected_index - table_layout::first_tableau_selection_index) * table_layout::pile_x_step);
    }

    int table_selection::_nearest_top_row_index_for_x(int x)
    {
        int best_index = table_layout::stock_selection_index;
        int best_distance = abs_distance(x, _pile_x_for_index(best_index));

        for(int index = table_layout::waste_selection_index; index <= table_layout::last_foundation_selection_index;
            ++index)
        {
            const int distance = abs_distance(x, _pile_x_for_index(index));
            if(distance < best_distance)
            {
                best_distance = distance;
                best_index = index;
            }
        }

        return best_index;
    }

    int table_selection::_nearest_tableau_index_for_x(int x)
    {
        int best_tableau = 0;
        int best_distance = abs_distance(x, table_layout::tableau_base_x + (best_tableau * table_layout::pile_x_step));

        for(int tableau_index = 1; tableau_index < 7; ++tableau_index)
        {
            const int distance = abs_distance(x, table_layout::tableau_base_x + (tableau_index * table_layout::pile_x_step));
            if(distance < best_distance)
            {
                best_distance = distance;
                best_tableau = tableau_index;
            }
        }

        return table_layout::first_tableau_selection_index + best_tableau;
    }

}
