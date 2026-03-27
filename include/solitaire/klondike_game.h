#ifndef SOLITAIRE_KLONDIKE_GAME_H
#define SOLITAIRE_KLONDIKE_GAME_H

#include <array>

#include "bn_random.h"
#include "bn_vector.h"

#include "solitaire/card.h"

namespace solitaire
{

    enum class pile_kind
    {
        stock,
        waste,
        foundation,
        tableau,
    };

    struct pile_ref
    {
        pile_kind kind = pile_kind::stock;
        int index = 0;
    };

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
        struct tableau_pile
        {
            bn::vector<card, 19> face_down;
            bn::vector<card, 19> face_up;
        };

        void _deal_new_game();
        void _flip_tableau_if_needed(int tableau_index);

        [[nodiscard]] bool _can_place_on_foundation(const card& moving_card, int foundation_index) const;
        [[nodiscard]] bool _can_place_on_tableau(const card& moving_card, int tableau_index) const;

        bn::vector<card, 52> _stock;
        bn::vector<card, 52> _waste;
        std::array<bn::vector<card, 13>, 4> _foundations;
        std::array<tableau_pile, 7> _tableaus;

        bn::random _random;

        bool _has_held_card = false;
        bn::vector<card, 19> _held_cards;
        pile_ref _held_from;
        bool _pending_tableau_flip = false;
        int _pending_tableau_flip_index = 0;
    };

}

#endif
