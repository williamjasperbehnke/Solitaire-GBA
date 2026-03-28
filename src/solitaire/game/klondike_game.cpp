#include "solitaire/game/klondike_game.h"

namespace solitaire
{
    klondike_game::klondike_game() = default;

    void klondike_game::set_seed(unsigned seed)
    {
        _seed = seed == 0 ? 1u : seed;
    }

    void klondike_game::reset()
    {
        _deal_new_game();
    }

    void klondike_game::setup_debug_one_move_to_win()
    {
        _stock.clear();
        _waste.clear();
        for(auto& foundation : _foundations)
        {
            foundation.clear();
        }
        for(auto& tableau : _tableaus)
        {
            tableau.face_down.clear();
            tableau.face_up.clear();
        }

        // Build three complete foundations and one missing only the King.
        for(int suit_index = 0; suit_index < 4; ++suit_index)
        {
            const int max_rank = suit_index == int(suit::hearts) ? 12 : 13;
            for(int rank = 1; rank <= max_rank; ++rank)
            {
                _foundations[suit_index].push_back(card { rank, static_cast<suit>(suit_index) });
            }
        }

        // One move away from victory: move KH from waste to the hearts foundation.
        _waste.push_back(card { 13, suit::hearts });
        _held_state.reset_for_new_game();
    }

    bool klondike_game::is_winnable_with_search(int node_budget) const
    {
        klondike_winnability_filter filter;
        return filter.is_likely_winnable(_stock, _waste, _foundations, _tableaus, node_budget);
    }

    deal_quality klondike_game::classify_deal_with_search(int easy_win_depth_limit, int hard_search_depth_limit,
                                                          int easy_search_node_budget,
                                                          int hard_search_node_budget) const
    {
        klondike_winnability_filter filter;
        return filter.classify_deal(_stock, _waste, _foundations, _tableaus, easy_win_depth_limit,
                                    hard_search_depth_limit, easy_search_node_budget, hard_search_node_budget);
    }

    bool klondike_game::draw_from_stock()
    {
        if(! _stock.empty())
        {
            _waste.push_back(_stock.back());
            _stock.pop_back();
            return true;
        }

        if(_waste.empty())
        {
            return false;
        }

        while(! _waste.empty())
        {
            _stock.push_back(_waste.back());
            _waste.pop_back();
        }

        return true;
    }

    bool klondike_game::try_waste_to_foundation(waste_to_foundation_move& out_move)
    {
        out_move = {};

        if(_waste.empty())
        {
            return false;
        }

        const card candidate = _waste.back();
        out_move.had_previous_card = false;
        for(int index = 0; index < 4; ++index)
        {
            if(_rules.can_place_on_foundation(candidate, _foundations[index]))
            {
                out_move.had_previous_card = ! _foundations[index].empty();
                if(out_move.had_previous_card)
                {
                    out_move.previous_card = _foundations[index].back();
                }
                _waste.pop_back();
                _foundations[index].push_back(candidate);
                out_move.foundation_index = index;
                out_move.moved_card = candidate;
                return true;
            }
        }

        return false;
    }

    bool klondike_game::try_pick(const pile_ref& from, int tableau_depth_from_top)
    {
        if(_held_state.has_held_card())
        {
            return false;
        }

        _held_state.clear_before_pick();
        auto& held_cards = _held_state.held_cards();

        switch(from.kind)
        {
            case pile_kind::stock:
                return false;

            case pile_kind::waste:
                if(_waste.empty())
                {
                    return false;
                }
                held_cards.push_back(_waste.back());
                _waste.pop_back();
                break;

            case pile_kind::foundation:
            {
                auto& foundation = _foundations[from.index];
                if(foundation.empty())
                {
                    return false;
                }
                held_cards.push_back(foundation.back());
                foundation.pop_back();
                break;
            }

            case pile_kind::tableau:
            {
                auto& tableau = _tableaus[from.index];
                const int face_up_size = tableau.face_up.size();
                if(face_up_size <= 0)
                {
                    return false;
                }

                if(tableau_depth_from_top < 0 || tableau_depth_from_top >= face_up_size)
                {
                    return false;
                }

                const int pickup_index = face_up_size - 1 - tableau_depth_from_top;
                for(int index = pickup_index; index < face_up_size; ++index)
                {
                    held_cards.push_back(tableau.face_up[index]);
                }

                while(tableau.face_up.size() > pickup_index)
                {
                    tableau.face_up.pop_back();
                }

                if(tableau.face_up.empty() && ! tableau.face_down.empty())
                {
                    _held_state.set_pending_tableau_flip(from.index);
                }
                break;
            }
            default:
                return false;
        }

        _held_state.begin_hold_from(from);
        return true;
    }

    bool klondike_game::try_place(const pile_ref& to)
    {
        if(! _held_state.has_held_card())
        {
            return false;
        }

        if(to.kind == _held_state.held_from().kind && to.index == _held_state.held_from().index)
        {
            cancel_held();
            return true;
        }

        auto& held_cards = _held_state.held_cards();
        const card& moving_base_card = held_cards.front();

        switch(to.kind)
        {
            case pile_kind::stock:
                return false;

            case pile_kind::waste:
                return false;

            case pile_kind::foundation:
            {
                if(held_cards.size() != 1)
                {
                    return false;
                }

                if(! _rules.can_place_on_foundation(moving_base_card, _foundations[to.index]))
                {
                    return false;
                }

                _foundations[to.index].push_back(moving_base_card);
                _finish_successful_place();
                return true;
            }

            case pile_kind::tableau:
            {
                if(! _rules.can_place_on_tableau(moving_base_card, _tableaus[to.index]))
                {
                    return false;
                }

                auto& target_face_up = _tableaus[to.index].face_up;
                for(const card& value : held_cards)
                {
                    target_face_up.push_back(value);
                }

                _finish_successful_place();
                return true;
            }
            default:
                return false;
        }
    }

    void klondike_game::cancel_held()
    {
        if(! _held_state.has_held_card())
        {
            return;
        }

        const pile_ref& held_from = _held_state.held_from();
        const auto& held_cards = _held_state.held_cards();

        switch(held_from.kind)
        {
            case pile_kind::stock:
                for(const card& value : held_cards)
                {
                    _stock.push_back(value);
                }
                break;

            case pile_kind::waste:
                for(const card& value : held_cards)
                {
                    _waste.push_back(value);
                }
                break;

            case pile_kind::foundation:
                for(const card& value : held_cards)
                {
                    _foundations[held_from.index].push_back(value);
                }
                break;

            case pile_kind::tableau:
                for(const card& value : held_cards)
                {
                    _tableaus[held_from.index].face_up.push_back(value);
                }
                break;
            default:
                break;
        }

        _held_state.clear_all();
    }

    bool klondike_game::has_held_card() const
    {
        return _held_state.has_held_card();
    }

    const card& klondike_game::held_card(int index) const
    {
        return _held_state.held_cards()[index];
    }

    int klondike_game::held_cards_count() const
    {
        return _held_state.held_cards().size();
    }

    const pile_ref& klondike_game::held_from() const
    {
        return _held_state.held_from();
    }

    int klondike_game::stock_size() const
    {
        return _stock.size();
    }

    bool klondike_game::stock_top(card& out_card) const
    {
        if(_stock.empty())
        {
            return false;
        }

        out_card = _stock.back();
        return true;
    }

    int klondike_game::waste_size() const
    {
        return _waste.size();
    }

    bool klondike_game::waste_card_from_top(int offset, card& out_card) const
    {
        const int waste_cards_count = _waste.size();
        const int target_index = waste_cards_count - 1 - offset;
        if(target_index < 0 || target_index >= waste_cards_count)
        {
            return false;
        }

        out_card = _waste[target_index];
        return true;
    }

    int klondike_game::foundation_size(int index) const
    {
        return _foundations[index].size();
    }

    bool klondike_game::foundation_top(int index, card& out_card) const
    {
        const auto& foundation = _foundations[index];
        if(foundation.empty())
        {
            return false;
        }

        out_card = foundation.back();
        return true;
    }

    int klondike_game::tableau_face_down_size(int index) const
    {
        return _tableaus[index].face_down.size();
    }

    int klondike_game::tableau_face_up_size(int index) const
    {
        return _tableaus[index].face_up.size();
    }

    bool klondike_game::tableau_face_up_card(int tableau_index, int face_up_index, card& out_card) const
    {
        const auto& face_up = _tableaus[tableau_index].face_up;
        if(face_up_index < 0 || face_up_index >= face_up.size())
        {
            return false;
        }

        out_card = face_up[face_up_index];
        return true;
    }

    bool klondike_game::has_won() const
    {
        for(const auto& foundation : _foundations)
        {
            if(foundation.size() != 13)
            {
                return false;
            }
        }

        return true;
    }

    void klondike_game::_deal_new_game()
    {
        _random.set_seed(_seed);
        _dealer.deal_new_game(_random, _stock, _waste, _foundations, _tableaus);
        _held_state.reset_for_new_game();
    }

    void klondike_game::_flip_tableau_if_needed(int tableau_index)
    {
        auto& tableau = _tableaus[tableau_index];
        if(tableau.face_up.empty() && ! tableau.face_down.empty())
        {
            tableau.face_up.push_back(tableau.face_down.back());
            tableau.face_down.pop_back();
        }
    }

    void klondike_game::_finish_successful_place()
    {
        _held_state.finish_successful_place();
        if(_held_state.pending_tableau_flip())
        {
            _flip_tableau_if_needed(_held_state.pending_tableau_flip_index());
            _held_state.clear_pending_tableau_flip();
        }
    }

}
