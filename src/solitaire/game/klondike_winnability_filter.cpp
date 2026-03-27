#include "solitaire/game/klondike_winnability_filter.h"

#include "solitaire/game/klondike_rules.h"

namespace solitaire
{
    namespace
    {
        constexpr int recent_hash_window = 512;

        struct solver_state
        {
            stock_pile stock;
            waste_pile waste;
            foundation_piles foundations;
            tableau_piles tableaus;
        };

        struct solve_policy
        {
            bool reverse_tableau_scan = false;
            bool reverse_foundation_scan = false;
            bool prefer_reveal_moves = true;
            bool try_waste_before_tableau = false;
        };

        [[nodiscard]] unsigned hash_card(unsigned value, const card& current_card)
        {
            const unsigned encoded = unsigned(current_card.rank) | (unsigned(current_card.card_suit) << 4);
            return value ^ (encoded + 0x9E3779B9u + (value << 6) + (value >> 2));
        }

        [[nodiscard]] unsigned hash_state(const solver_state& state)
        {
            unsigned hash = 0x6A09E667u;

            for(const card& value : state.stock)
            {
                hash = hash_card(hash, value);
            }
            hash ^= 0xA5A5A5A5u;

            for(const card& value : state.waste)
            {
                hash = hash_card(hash, value);
            }
            hash ^= 0xC3C3C3C3u;

            for(const auto& foundation : state.foundations)
            {
                for(const card& value : foundation)
                {
                    hash = hash_card(hash, value);
                }
                hash = (hash << 1) ^ 0x1F123BB5u;
            }

            for(const auto& tableau : state.tableaus)
            {
                for(const card& value : tableau.face_down)
                {
                    hash = hash_card(hash, value);
                }
                hash ^= 0x5B5B5B5Bu;

                for(const card& value : tableau.face_up)
                {
                    hash = hash_card(hash, value);
                }
                hash = (hash << 1) ^ 0x7F4A7C15u;
            }

            return hash == 0 ? 1u : hash;
        }

        void flip_tableau_if_needed(tableau_pile& tableau)
        {
            if(tableau.face_up.empty() && ! tableau.face_down.empty())
            {
                tableau.face_up.push_back(tableau.face_down.back());
                tableau.face_down.pop_back();
            }
        }

        [[nodiscard]] bool state_has_won(const foundation_piles& foundations)
        {
            for(const auto& foundation : foundations)
            {
                if(foundation.size() != 13)
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] int iterate_index(int count, int pass_index, bool reverse)
        {
            return reverse ? (count - 1 - pass_index) : pass_index;
        }

        [[nodiscard]] bool try_waste_to_foundation(solver_state& state, const klondike_rules& rules,
                                                   const solve_policy& policy)
        {
            if(state.waste.empty())
            {
                return false;
            }

            const card moving_card = state.waste.back();
            for(int pass = 0; pass < 4; ++pass)
            {
                const int foundation_index = iterate_index(4, pass, policy.reverse_foundation_scan);
                if(rules.can_place_on_foundation(moving_card, state.foundations[foundation_index]))
                {
                    state.waste.pop_back();
                    state.foundations[foundation_index].push_back(moving_card);
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool try_tableau_to_foundation(solver_state& state, const klondike_rules& rules,
                                                     const solve_policy& policy)
        {
            for(int pass_tableau = 0; pass_tableau < 7; ++pass_tableau)
            {
                const int tableau_index = iterate_index(7, pass_tableau, policy.reverse_tableau_scan);
                auto& tableau = state.tableaus[tableau_index];
                if(tableau.face_up.empty())
                {
                    continue;
                }

                const card moving_card = tableau.face_up.back();
                for(int pass_foundation = 0; pass_foundation < 4; ++pass_foundation)
                {
                    const int foundation_index = iterate_index(4, pass_foundation, policy.reverse_foundation_scan);
                    if(rules.can_place_on_foundation(moving_card, state.foundations[foundation_index]))
                    {
                        tableau.face_up.pop_back();
                        flip_tableau_if_needed(tableau);
                        state.foundations[foundation_index].push_back(moving_card);
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool move_tableau_stack(solver_state& state, int from_index, int pickup_index, int to_index)
        {
            auto& from = state.tableaus[from_index];
            auto& to = state.tableaus[to_index];
            const int from_face_up_size = from.face_up.size();
            for(int index = pickup_index; index < from_face_up_size; ++index)
            {
                to.face_up.push_back(from.face_up[index]);
            }

            while(from.face_up.size() > pickup_index)
            {
                from.face_up.pop_back();
            }

            flip_tableau_if_needed(from);
            return true;
        }

        [[nodiscard]] bool try_tableau_to_tableau_reveal(solver_state& state, const klondike_rules& rules,
                                                         const solve_policy& policy)
        {
            for(int pass_from = 0; pass_from < 7; ++pass_from)
            {
                const int from_index = iterate_index(7, pass_from, policy.reverse_tableau_scan);
                const auto& from = state.tableaus[from_index];
                if(from.face_up.empty() || from.face_down.empty())
                {
                    continue;
                }

                const card moving_base = from.face_up.front();
                for(int pass_to = 0; pass_to < 7; ++pass_to)
                {
                    const int to_index = iterate_index(7, pass_to, policy.reverse_tableau_scan);
                    if(to_index == from_index)
                    {
                        continue;
                    }

                    if(rules.can_place_on_tableau(moving_base, state.tableaus[to_index]))
                    {
                        return move_tableau_stack(state, from_index, 0, to_index);
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool try_tableau_to_tableau_any(solver_state& state, const klondike_rules& rules,
                                                       const solve_policy& policy)
        {
            for(int pass_from = 0; pass_from < 7; ++pass_from)
            {
                const int from_index = iterate_index(7, pass_from, policy.reverse_tableau_scan);
                const auto& from = state.tableaus[from_index];
                const int face_up_count = from.face_up.size();
                if(face_up_count <= 0)
                {
                    continue;
                }

                for(int depth_from_top = 0; depth_from_top < face_up_count; ++depth_from_top)
                {
                    const int pickup_index = face_up_count - 1 - depth_from_top;
                    const card moving_base = from.face_up[pickup_index];

                    for(int pass_to = 0; pass_to < 7; ++pass_to)
                    {
                        const int to_index = iterate_index(7, pass_to, policy.reverse_tableau_scan);
                        if(to_index == from_index)
                        {
                            continue;
                        }

                        if(rules.can_place_on_tableau(moving_base, state.tableaus[to_index]))
                        {
                            return move_tableau_stack(state, from_index, pickup_index, to_index);
                        }
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool try_waste_to_tableau(solver_state& state, const klondike_rules& rules,
                                                const solve_policy& policy)
        {
            if(state.waste.empty())
            {
                return false;
            }

            const card moving_card = state.waste.back();
            for(int pass = 0; pass < 7; ++pass)
            {
                const int tableau_index = iterate_index(7, pass, policy.reverse_tableau_scan);
                if(rules.can_place_on_tableau(moving_card, state.tableaus[tableau_index]))
                {
                    state.waste.pop_back();
                    state.tableaus[tableau_index].face_up.push_back(moving_card);
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool draw_or_recycle(solver_state& state)
        {
            if(! state.stock.empty())
            {
                state.waste.push_back(state.stock.back());
                state.stock.pop_back();
                return true;
            }

            if(! state.waste.empty())
            {
                while(! state.waste.empty())
                {
                    state.stock.push_back(state.waste.back());
                    state.waste.pop_back();
                }
                return true;
            }

            return false;
        }

        [[nodiscard]] bool try_any_move(solver_state& state, const klondike_rules& rules, const solve_policy& policy)
        {
            if(policy.try_waste_before_tableau)
            {
                if(try_waste_to_foundation(state, rules, policy) || try_waste_to_tableau(state, rules, policy) ||
                   try_tableau_to_foundation(state, rules, policy) ||
                   (policy.prefer_reveal_moves && try_tableau_to_tableau_reveal(state, rules, policy)) ||
                   try_tableau_to_tableau_any(state, rules, policy))
                {
                    return true;
                }
            }
            else
            {
                if(try_tableau_to_foundation(state, rules, policy) || try_waste_to_foundation(state, rules, policy) ||
                   (policy.prefer_reveal_moves && try_tableau_to_tableau_reveal(state, rules, policy)) ||
                   try_tableau_to_tableau_any(state, rules, policy) || try_waste_to_tableau(state, rules, policy))
                {
                    return true;
                }
            }

            return draw_or_recycle(state);
        }

        [[nodiscard]] bool run_policy(const solver_state& initial_state, const solve_policy& policy,
                                      int step_limit, int node_budget)
        {
            solver_state state = initial_state;
            klondike_rules rules;

            unsigned recent_hashes[recent_hash_window] = {};
            int recent_count = 0;
            int recent_index = 0;

            for(int step = 0; step < step_limit && step < node_budget; ++step)
            {
                if(state_has_won(state.foundations))
                {
                    return true;
                }

                const unsigned current_hash = hash_state(state);
                bool repeated = false;
                for(int index = 0; index < recent_count; ++index)
                {
                    if(recent_hashes[index] == current_hash)
                    {
                        repeated = true;
                        break;
                    }
                }

                if(repeated)
                {
                    return false;
                }

                recent_hashes[recent_index] = current_hash;
                recent_index = (recent_index + 1) % recent_hash_window;
                if(recent_count < recent_hash_window)
                {
                    ++recent_count;
                }

                if(! try_any_move(state, rules, policy))
                {
                    return false;
                }
            }

            return state_has_won(state.foundations);
        }
    }

    deal_quality klondike_winnability_filter::classify_deal(const stock_pile& stock, const waste_pile& waste,
                                                            const foundation_piles& foundations,
                                                            const tableau_piles& tableaus,
                                                            int easy_win_depth_limit, int hard_search_depth_limit,
                                                            int easy_search_node_budget,
                                                            int hard_search_node_budget) const
    {
        solver_state initial_state;
        initial_state.stock = stock;
        initial_state.waste = waste;
        initial_state.foundations = foundations;
        initial_state.tableaus = tableaus;

        const solve_policy policies[] = {
            { false, false, true, false },
            { true, false, true, false },
            { false, true, true, true }
        };

        if(easy_win_depth_limit > 0 && easy_search_node_budget > 0)
        {
            for(const solve_policy& policy : policies)
            {
                if(run_policy(initial_state, policy, easy_win_depth_limit, easy_search_node_budget))
                {
                    return deal_quality::too_easy;
                }
            }
        }

        for(const solve_policy& policy : policies)
        {
            if(run_policy(initial_state, policy, hard_search_depth_limit, hard_search_node_budget))
            {
                return deal_quality::acceptable;
            }
        }

        return deal_quality::unwinnable_or_unknown;
    }

    bool klondike_winnability_filter::is_likely_winnable(const stock_pile& stock, const waste_pile& waste,
                                                         const foundation_piles& foundations,
                                                         const tableau_piles& tableaus, int step_budget) const
    {
        const deal_quality quality = classify_deal(stock, waste, foundations, tableaus, 0, step_budget, 0,
                                                   step_budget);
        return quality == deal_quality::acceptable;
    }

}
