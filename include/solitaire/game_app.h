#ifndef SOLITAIRE_GAME_APP_H
#define SOLITAIRE_GAME_APP_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_timer.h"
#include "bn_vector.h"

#include "solitaire/klondike_game.h"

namespace solitaire
{

    class game_app
    {
    public:
        game_app();

        void update();

    private:
        [[nodiscard]] pile_ref _selected_pile() const;

        void _update_input();
        void _render();
        void _draw_card_sprite(const card& value, int x, int y);
        void _draw_card_back_sprite(int x, int y);
        void _draw_selection_highlight(int x, int y, bool use_waste_style = false);
        [[nodiscard]] static bn::string<32> _time_moves_text(int elapsed_ticks, int moves_count);

        klondike_game _game;
        int _selected_index = 0;
        int _tableau_pick_depth_from_top = 0;
        bn::timer _time_timer;
        int _elapsed_ticks = 0;
        int _moves_count = 0;
        bn::regular_bg_ptr _table_bg;

        bn::sprite_text_generator _text_generator;
        bn::vector<bn::sprite_ptr, 256> _text_sprites;
        bn::vector<bn::sprite_ptr, 128> _card_sprites;
    };

}

#endif
