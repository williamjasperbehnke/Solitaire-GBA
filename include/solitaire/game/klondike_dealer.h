#ifndef SOLITAIRE_KLONDIKE_DEALER_H
#define SOLITAIRE_KLONDIKE_DEALER_H

#include "bn_random.h"

#include "solitaire/game/klondike_types.h"

namespace solitaire
{

    class klondike_dealer
    {
    public:
        void deal_new_game(bn::random& random, stock_pile& stock, waste_pile& waste,
                           foundation_piles& foundations, tableau_piles& tableaus) const;

    private:
        static void _clear_piles(stock_pile& stock, waste_pile& waste, foundation_piles& foundations, tableau_piles& tableaus);
        static void _fill_stock_with_new_deck(stock_pile& stock);
        static void _shuffle_stock(bn::random& random, stock_pile& stock);
        static void _deal_tableau(stock_pile& stock, tableau_piles& tableaus);
        static void _apply_render_stress_test(stock_pile& stock, tableau_piles& tableaus);
    };

}

#endif
