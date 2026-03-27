#include "solitaire/klondike_game.h"

namespace solitaire
{

    klondike_game::klondike_game()
    {
        _deal_new_game();
    }

    void klondike_game::reset()
    {
        _deal_new_game();
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

    bool klondike_game::try_pick(const pile_ref& from, int tableau_depth_from_top)
    {
        if(_has_held_card)
        {
            return false;
        }

        _pending_tableau_flip = false;
        _held_cards.clear();

        switch(from.kind)
        {
            case pile_kind::stock:
                return false;

            case pile_kind::waste:
                if(_waste.empty())
                {
                    return false;
                }
                _held_cards.push_back(_waste.back());
                _waste.pop_back();
                break;

            case pile_kind::foundation:
            {
                auto& foundation = _foundations[from.index];
                if(foundation.empty())
                {
                    return false;
                }
                _held_cards.push_back(foundation.back());
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
                    _held_cards.push_back(tableau.face_up[index]);
                }

                while(tableau.face_up.size() > pickup_index)
                {
                    tableau.face_up.pop_back();
                }

                if(tableau.face_up.empty() && ! tableau.face_down.empty())
                {
                    _pending_tableau_flip = true;
                    _pending_tableau_flip_index = from.index;
                }
                break;
            }
            default:
                return false;
        }

        _has_held_card = true;
        _held_from = from;
        return true;
    }

    bool klondike_game::try_place(const pile_ref& to)
    {
        if(! _has_held_card)
        {
            return false;
        }

        if(to.kind == _held_from.kind && to.index == _held_from.index)
        {
            cancel_held();
            return true;
        }

        const card& moving_base_card = _held_cards.front();

        switch(to.kind)
        {
            case pile_kind::stock:
                return false;

            case pile_kind::waste:
                return false;

            case pile_kind::foundation:
            {
                if(_held_cards.size() != 1)
                {
                    return false;
                }

                if(! _can_place_on_foundation(moving_base_card, to.index))
                {
                    return false;
                }

                _foundations[to.index].push_back(moving_base_card);
                _held_cards.clear();
                _has_held_card = false;
                if(_pending_tableau_flip)
                {
                    _flip_tableau_if_needed(_pending_tableau_flip_index);
                    _pending_tableau_flip = false;
                }
                return true;
            }

            case pile_kind::tableau:
            {
                if(! _can_place_on_tableau(moving_base_card, to.index))
                {
                    return false;
                }

                auto& target_face_up = _tableaus[to.index].face_up;
                for(const card& value : _held_cards)
                {
                    target_face_up.push_back(value);
                }

                _held_cards.clear();
                _has_held_card = false;
                if(_pending_tableau_flip)
                {
                    _flip_tableau_if_needed(_pending_tableau_flip_index);
                    _pending_tableau_flip = false;
                }
                return true;
            }
            default:
                return false;
        }
    }

    void klondike_game::cancel_held()
    {
        if(! _has_held_card)
        {
            return;
        }

        switch(_held_from.kind)
        {
            case pile_kind::stock:
                for(const card& value : _held_cards)
                {
                    _stock.push_back(value);
                }
                break;

            case pile_kind::waste:
                for(const card& value : _held_cards)
                {
                    _waste.push_back(value);
                }
                break;

            case pile_kind::foundation:
                for(const card& value : _held_cards)
                {
                    _foundations[_held_from.index].push_back(value);
                }
                break;

            case pile_kind::tableau:
                for(const card& value : _held_cards)
                {
                    _tableaus[_held_from.index].face_up.push_back(value);
                }
                break;
            default:
                break;
        }

        _held_cards.clear();
        _has_held_card = false;
        _pending_tableau_flip = false;
    }

    bool klondike_game::has_held_card() const
    {
        return _has_held_card;
    }

    const card& klondike_game::held_card(int index) const
    {
        return _held_cards[index];
    }

    int klondike_game::held_cards_count() const
    {
        return _held_cards.size();
    }

    const pile_ref& klondike_game::held_from() const
    {
        return _held_from;
    }

    int klondike_game::stock_size() const
    {
        return _stock.size();
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

        _has_held_card = false;
        _held_cards.clear();
        _pending_tableau_flip = false;

        for(int suit_index = 0; suit_index < 4; ++suit_index)
        {
            for(int rank = 1; rank <= 13; ++rank)
            {
                _stock.push_back(card { rank, static_cast<suit>(suit_index) });
            }
        }

        for(int index = _stock.size() - 1; index > 0; --index)
        {
            int swap_index = _random.get_int(index + 1);
            card temp = _stock[index];
            _stock[index] = _stock[swap_index];
            _stock[swap_index] = temp;
        }

        for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
        {
            auto& tableau = _tableaus[tableau_index];

            for(int down_count = 0; down_count < tableau_index; ++down_count)
            {
                tableau.face_down.push_back(_stock.back());
                _stock.pop_back();
            }

            tableau.face_up.push_back(_stock.back());
            _stock.pop_back();
        }
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

    bool klondike_game::_can_place_on_foundation(const card& moving_card, int foundation_index) const
    {
        const auto& foundation = _foundations[foundation_index];
        if(foundation.empty())
        {
            return moving_card.rank == 1;
        }

        const card& top_card = foundation.back();
        return moving_card.card_suit == top_card.card_suit && moving_card.rank == (top_card.rank + 1);
    }

    bool klondike_game::_can_place_on_tableau(const card& moving_card, int tableau_index) const
    {
        const auto& face_up = _tableaus[tableau_index].face_up;
        if(face_up.empty())
        {
            return moving_card.rank == 13;
        }

        const card& top_card = face_up.back();
        return moving_card.rank == (top_card.rank - 1) && moving_card.is_red() != top_card.is_red();
    }

}
