#ifndef SOLITAIRE_KLONDIKE_TYPES_H
#define SOLITAIRE_KLONDIKE_TYPES_H

#include <array>

#include "bn_vector.h"

#include "solitaire/core/card.h"

namespace solitaire
{

    struct tableau_pile
    {
        bn::vector<card, 19> face_down;
        bn::vector<card, 19> face_up;
    };

    using stock_pile = bn::vector<card, 52>;
    using waste_pile = bn::vector<card, 52>;
    using foundation_piles = std::array<bn::vector<card, 13>, 4>;
    using tableau_piles = std::array<tableau_pile, 7>;

}

#endif
