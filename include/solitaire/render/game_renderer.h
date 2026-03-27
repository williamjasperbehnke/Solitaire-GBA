#ifndef SOLITAIRE_GAME_RENDERER_H
#define SOLITAIRE_GAME_RENDERER_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "solitaire/game/klondike_game.h"
#include "solitaire/input/table_selection.h"

namespace solitaire
{

    class game_renderer
    {
    public:
        game_renderer();

        void render(const klondike_game& game, const table_selection& selection, int elapsed_ticks, int moves_count,
                    bool show_press_start_prompt, bool show_deal_animation, int deal_animation_frame,
                    unsigned animation_frame);

    private:
        void _render_top_row(const klondike_game& game, const table_selection& selection, bool lift_selected_card,
                             card& top_card);
        void _render_tableau(const klondike_game& game, const table_selection& selection, bool lift_selected_card,
                             card& top_card);
        void _render_held_cards(const klondike_game& game);
        void _render_status_message(const klondike_game& game);
        void _render_press_start_prompt(unsigned animation_frame);
        void _render_deal_animation(const klondike_game& game, int deal_animation_frame);

        void _draw_held_cards_panel(int held_cards_count);
        void _draw_panel_tile_pair(int x, int top_y, int bottom_y, int top_graphics_index, int bottom_graphics_index);
        void _draw_card_sprite(const card& value, int x, int y);
        void _draw_held_card_sprite(const card& value, int x, int y);
        void _draw_card_back_sprite(int x, int y);
        void _update_selection_highlight(int x, int y, bool use_waste_style);

        [[nodiscard]] static bn::string<32> _time_moves_text(int elapsed_ticks, int moves_count);

        bn::regular_bg_ptr _table_bg;
        bn::regular_bg_ptr _slot_highlight_bg;
        bn::regular_bg_ptr _waste_highlight_bg;

        bn::sprite_text_generator _text_generator;
        bn::vector<bn::sprite_ptr, 256> _text_sprites;
        bn::vector<bn::sprite_ptr, 128> _card_sprites;
    };

}

#endif
