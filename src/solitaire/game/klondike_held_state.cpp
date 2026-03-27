#include "solitaire/game/klondike_held_state.h"

namespace solitaire
{

    void klondike_held_state::reset_for_new_game()
    {
        _has_held_card = false;
        _held_cards.clear();
        _pending_tableau_flip = false;
    }

    void klondike_held_state::clear_before_pick()
    {
        _pending_tableau_flip = false;
        _held_cards.clear();
    }

    void klondike_held_state::begin_hold_from(const pile_ref& from)
    {
        _has_held_card = true;
        _held_from = from;
    }

    void klondike_held_state::finish_successful_place()
    {
        _held_cards.clear();
        _has_held_card = false;
    }

    void klondike_held_state::clear_all()
    {
        _held_cards.clear();
        _has_held_card = false;
        _pending_tableau_flip = false;
    }

    void klondike_held_state::set_pending_tableau_flip(int tableau_index)
    {
        _pending_tableau_flip = true;
        _pending_tableau_flip_index = tableau_index;
    }

    void klondike_held_state::clear_pending_tableau_flip()
    {
        _pending_tableau_flip = false;
    }

    bool klondike_held_state::has_held_card() const
    {
        return _has_held_card;
    }

    bn::vector<card, 19>& klondike_held_state::held_cards()
    {
        return _held_cards;
    }

    const bn::vector<card, 19>& klondike_held_state::held_cards() const
    {
        return _held_cards;
    }

    const pile_ref& klondike_held_state::held_from() const
    {
        return _held_from;
    }

    bool klondike_held_state::pending_tableau_flip() const
    {
        return _pending_tableau_flip;
    }

    int klondike_held_state::pending_tableau_flip_index() const
    {
        return _pending_tableau_flip_index;
    }

}
