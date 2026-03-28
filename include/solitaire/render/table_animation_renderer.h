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
    };

}

#endif
