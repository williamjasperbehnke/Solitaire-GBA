#ifndef SOLITAIRE_KLONDIKE_HELD_STATE_H
#define SOLITAIRE_KLONDIKE_HELD_STATE_H

#include "bn_vector.h"

#include "solitaire/core/card.h"
#include "solitaire/core/pile_ref.h"

namespace solitaire
{

    class klondike_held_state
    {
    public:
        void reset_for_new_game();
        void clear_before_pick();
        void begin_hold_from(const pile_ref& from);
        void finish_successful_place();
        void clear_all();

        void set_pending_tableau_flip(int tableau_index);
        void clear_pending_tableau_flip();

        [[nodiscard]] bool has_held_card() const;
        [[nodiscard]] bn::vector<card, 19>& held_cards();
        [[nodiscard]] const bn::vector<card, 19>& held_cards() const;
        [[nodiscard]] const pile_ref& held_from() const;
        [[nodiscard]] bool pending_tableau_flip() const;
        [[nodiscard]] int pending_tableau_flip_index() const;

    private:
        bool _has_held_card = false;
        bn::vector<card, 19> _held_cards;
        pile_ref _held_from;
        bool _pending_tableau_flip = false;
        int _pending_tableau_flip_index = 0;
    };

}

#endif
