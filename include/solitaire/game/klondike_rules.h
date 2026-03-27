#ifndef SOLITAIRE_KLONDIKE_RULES_H
#define SOLITAIRE_KLONDIKE_RULES_H

#include "solitaire/core/card.h"
#include "solitaire/game/klondike_types.h"

namespace solitaire
{

    class klondike_rules
    {
    public:
        [[nodiscard]] bool can_place_on_foundation(const card& moving_card, const bn::vector<card, 13>& foundation) const;
        [[nodiscard]] bool can_place_on_tableau(const card& moving_card, const tableau_pile& tableau) const;
    };

}

#endif
