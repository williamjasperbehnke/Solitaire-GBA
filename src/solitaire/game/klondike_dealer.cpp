#include "solitaire/game/klondike_dealer.h"

namespace solitaire
{

    void klondike_dealer::deal_new_game(bn::seed_random& random, stock_pile& stock, waste_pile& waste,
                                        foundation_piles& foundations, tableau_piles& tableaus) const
    {
        _clear_piles(stock, waste, foundations, tableaus);
        _fill_stock_with_new_deck(stock);
        _shuffle_stock(random, stock);
        _deal_tableau(stock, tableaus);
    }

    void klondike_dealer::_clear_piles(stock_pile& stock, waste_pile& waste, foundation_piles& foundations,
                                       tableau_piles& tableaus)
    {
        stock.clear();
        waste.clear();
        for(auto& foundation : foundations)
        {
            foundation.clear();
        }
        for(auto& tableau : tableaus)
        {
            tableau.face_down.clear();
            tableau.face_up.clear();
        }
    }

    void klondike_dealer::_fill_stock_with_new_deck(stock_pile& stock)
    {
        for(int suit_index = 0; suit_index < 4; ++suit_index)
        {
            for(int rank = 1; rank <= 13; ++rank)
            {
                stock.push_back(card { rank, static_cast<suit>(suit_index) });
            }
        }
    }

    void klondike_dealer::_shuffle_stock(bn::seed_random& random, stock_pile& stock)
    {
        for(int index = stock.size() - 1; index > 0; --index)
        {
            const int swap_index = random.get_int(index + 1);
            const card temp = stock[index];
            stock[index] = stock[swap_index];
            stock[swap_index] = temp;
        }
    }

    void klondike_dealer::_deal_tableau(stock_pile& stock, tableau_piles& tableaus)
    {
        // Standard Klondike deal order: row by row across columns.
        for(int row = 0; row < 7; ++row)
        {
            for(int tableau_index = row; tableau_index < 7; ++tableau_index)
            {
                auto& tableau = tableaus[tableau_index];
                const card dealt_card = stock.back();
                stock.pop_back();

                if(row == tableau_index)
                {
                    tableau.face_up.push_back(dealt_card);
                }
                else
                {
                    tableau.face_down.push_back(dealt_card);
                }
            }
        }
    }

}
