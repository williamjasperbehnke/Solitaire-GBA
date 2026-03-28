#ifndef SOLITAIRE_KLONDIKE_GAME_H
#define SOLITAIRE_KLONDIKE_GAME_H

#include <array>

#include "bn_seed_random.h"
#include "bn_vector.h"

#include "solitaire/core/card.h"
#include "solitaire/game/klondike_dealer.h"
#include "solitaire/game/klondike_held_state.h"
#include "solitaire/game/klondike_rules.h"
#include "solitaire/game/klondike_types.h"
#include "solitaire/game/klondike_winnability_filter.h"
#include "solitaire/core/pile_ref.h"

namespace solitaire
{

    class klondike_game
    {
    public:
        struct waste_to_foundation_move
        {
            int foundation_index = -1;
            card moved_card = {};
            bool had_previous_card = false;
            card previous_card = {};
        };

        klondike_game();

        void set_seed(unsigned seed);
        void reset();
        void setup_debug_one_move_to_win();
        void setup_debug_tableau_spacing_example();
        [[nodiscard]] bool is_winnable_with_search(int node_budget) const;
        [[nodiscard]] deal_quality classify_deal_with_search(int easy_win_depth_limit, int hard_search_depth_limit,
                                                             int easy_search_node_budget,
                                                             int hard_search_node_budget) const;

        [[nodiscard]] bool draw_from_stock();
        [[nodiscard]] bool try_waste_to_foundation(waste_to_foundation_move& out_move);
        [[nodiscard]] bool try_pick(const pile_ref& from, int tableau_depth_from_top = 0);
        [[nodiscard]] bool try_place(const pile_ref& to);
        void cancel_held();

        [[nodiscard]] bool has_held_card() const;
        [[nodiscard]] const card& held_card(int index) const;
        [[nodiscard]] int held_cards_count() const;
        [[nodiscard]] const pile_ref& held_from() const;

        [[nodiscard]] int stock_size() const;
        [[nodiscard]] bool stock_top(card& out_card) const;
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
        void _finish_successful_place();

        stock_pile _stock;
        waste_pile _waste;
        foundation_piles _foundations;
        tableau_piles _tableaus;

        klondike_dealer _dealer;
        klondike_rules _rules;
        klondike_held_state _held_state;
        bn::seed_random _random;
        unsigned _seed = 1;
    };

}

#endif
