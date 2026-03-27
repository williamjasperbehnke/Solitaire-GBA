#ifndef SOLITAIRE_CARD_SPRITE_REGISTRY_H
#define SOLITAIRE_CARD_SPRITE_REGISTRY_H

#include "bn_sprite_item.h"

#include "solitaire/card.h"

namespace solitaire
{

    const bn::sprite_item& card_back_item();
    const bn::sprite_item& card_face_item(const card& value);

}

#endif
