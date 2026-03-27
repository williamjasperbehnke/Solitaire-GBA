#ifndef SOLITAIRE_KLONDIKE_GAME_H
#define SOLITAIRE_KLONDIKE_GAME_H

#include <array>

#include "bn_random.h"
#include "bn_vector.h"

#include "solitaire/core/card.h"
#include "solitaire/game/klondike_dealer.h"
#include "solitaire/game/klondike_held_state.h"
#include "solitaire/game/klondike_rules.h"
#include "solitaire/game/klondike_types.h"
#include "solitaire/core/pile_ref.h"

namespace solitaire
{

    class klondike_game
    {
    public:
        klondike_game();

        void reset();

        [[nodiscard]] bool draw_from_stock();
        [[nodiscard]] bool try_pick(const pile_ref& from, int tableau_depth_from_top = 0);
        [[nodiscard]] bool try_place(const pile_ref& to);
        void cancel_held();

        [[nodiscard]] bool has_held_card() const;
        [[nodiscard]] const card& held_card(int index) const;
        [[nodiscard]] int held_cards_count() const;
        [[nodiscard]] const pile_ref& held_from() const;

        [[nodiscard]] int stock_size() const;
        [[nodiscard]] int waste_size() const;
        [[nodiscard]] bool waste_card_from_top(int offset, card& out_card) const;

        [[nodiscard]] int foundation_size(int index) const;
        [[nodiscard]] bool foundation_top(int index, card& out_card) const;

        [[nodiscard]] int tableau_face_down_size(int index) const;
        [[nodiscard]] int tableau_face_up_size(int index) const;
        [[nodiscard]] bool tableau_face_up_card(int tableau_index, int face_up_index, card& out_card) const;

        [[nodiscard]] bool has_won() const;

    private:
        void _deal_new_game();
        void _flip_tableau_if_needed(int tableau_index);

        stock_pile _stock;
        waste_pile _waste;
        foundation_piles _foundations;
        tableau_piles _tableaus;

        klondike_dealer _dealer;
        klondike_rules _rules;
        klondike_held_state _held_state;
        bn::random _random;
    };

}

#endif
