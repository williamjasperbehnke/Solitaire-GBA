#include "solitaire/render/held_panel_layout.h"

namespace solitaire
{
    namespace
    {
        constexpr int panel_tile_size = 16;
        constexpr int panel_tile_half = panel_tile_size / 2;
        constexpr int panel_left_edge = -115;
        constexpr int panel_half_span = 14;
        constexpr int panel_growth_delay_px = 8;
    }

    held_panel_layout::geometry held_panel_layout::build(int held_cards_count)
    {
        geometry output;
        output.tile_count = _tile_count(held_cards_count, 8);
        output.first_tile_x = _first_tile_x(output.tile_count);
        output.center_x = output.first_tile_x + ((output.tile_count - 1) * panel_tile_size) / 2;
        return output;
    }

    int held_panel_layout::first_card_x(int held_cards_count, int card_step)
    {
        const geometry panel = build(held_cards_count);
        return panel.center_x - ((held_cards_count - 1) * card_step) / 2;
    }

    int held_panel_layout::_tile_count(int held_cards_count, int card_step)
    {
        const int cards_span = (held_cards_count - 1) * card_step;
        int panel_width = cards_span + (panel_half_span * 2) - panel_growth_delay_px;
        if(panel_width < 1)
        {
            panel_width = 1;
        }

        int tile_count = (panel_width + panel_tile_size - 1) / panel_tile_size;
        if(tile_count < 2)
        {
            tile_count = 2;
        }
        return tile_count;
    }

    int held_panel_layout::_first_tile_x(int tile_count)
    {
        int first_tile_x = panel_left_edge + panel_tile_half;
        const int max_first_tile_x = (120 - panel_tile_half) - ((tile_count - 1) * panel_tile_size);
        if(first_tile_x > max_first_tile_x)
        {
            first_tile_x = max_first_tile_x;
        }
        return first_tile_x;
    }

}
