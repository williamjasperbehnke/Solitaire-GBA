#ifndef SOLITAIRE_TABLE_ANIMATION_RENDERER_H
#define SOLITAIRE_TABLE_ANIMATION_RENDERER_H

#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "solitaire/game/klondike_game.h"

namespace solitaire
{

    class table_animation_renderer
    {
    public:
        void render_deal(const klondike_game& game, int deal_animation_frame,
                         bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const;

        void render_cancel(const klondike_game& game, int cancel_animation_frame,
                           bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const;

        void render_victory(const klondike_game& game, int victory_animation_frame,
                            bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const;

        void render_stock_to_waste(const card& value, int animation_frame, int total_frames,
                                   bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const;
        void render_waste_to_stock(int animation_frame, int total_frames,
                                   bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const;
        void render_card_flight(const card& value, int source_x, int source_y, int target_x, int target_y,
                                int animation_frame, int total_frames,
                                bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const;

    };

}

#endif
