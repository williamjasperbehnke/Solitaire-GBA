#ifndef SOLITAIRE_HELD_PANEL_LAYOUT_H
#define SOLITAIRE_HELD_PANEL_LAYOUT_H

namespace solitaire
{

    class held_panel_layout
    {
    public:
        struct geometry
        {
            int tile_count = 0;
            int first_tile_x = 0;
            int center_x = 0;
        };

        [[nodiscard]] static geometry build(int held_cards_count);
        [[nodiscard]] static int first_card_x(int held_cards_count, int card_step);

    private:
        [[nodiscard]] static int _tile_count(int held_cards_count, int card_step);
        [[nodiscard]] static int _first_tile_x(int tile_count);
    };

}

#endif
