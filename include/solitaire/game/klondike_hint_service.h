#ifndef SOLITAIRE_KLONDIKE_HINT_SERVICE_H
#define SOLITAIRE_KLONDIKE_HINT_SERVICE_H

#include "bn_string.h"

#include "solitaire/core/pile_ref.h"
#include "solitaire/game/klondike_game.h"

namespace solitaire
{

    class klondike_hint_service
    {
    public:
        struct hint_result
        {
            bn::string<48> text;
            bool has_selection = false;
            pile_ref selection = { pile_kind::stock, 0 };
            int tableau_pick_depth_from_top = 0;
            bool has_move = false;
            pile_ref move_from = { pile_kind::stock, 0 };
            pile_ref move_to = { pile_kind::stock, 0 };
        };

        [[nodiscard]] hint_result build_hint(const klondike_game& game) const;
    };

}

#endif
