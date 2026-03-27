#include "solitaire/game/klondike_rules.h"

namespace solitaire
{

    bool klondike_rules::can_place_on_foundation(const card& moving_card, const bn::vector<card, 13>& foundation) const
    {
        if(foundation.empty())
        {
            return moving_card.rank == 1;
        }

        const card& top_card = foundation.back();
        return moving_card.card_suit == top_card.card_suit && moving_card.rank == (top_card.rank + 1);
    }

    bool klondike_rules::can_place_on_tableau(const card& moving_card, const tableau_pile& tableau) const
    {
        if(tableau.face_up.empty())
        {
            return moving_card.rank == 13;
        }

        const card& top_card = tableau.face_up.back();
        return moving_card.rank == (top_card.rank - 1) && moving_card.is_red() != top_card.is_red();
    }

}
