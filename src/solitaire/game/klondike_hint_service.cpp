#include "solitaire/game/klondike_hint_service.h"

namespace solitaire
{
    namespace
    {
        constexpr int tableau_progress_lookahead_depth = 4;

        struct tableau_hint_state
        {
            int face_down_count = 0;
            bn::vector<card, 19> face_up;
        };

        struct hint_state
        {
            std::array<tableau_hint_state, 7> tableaus;
            std::array<bool, 4> foundation_has_top = {};
            std::array<card, 4> foundation_top = {};
            std::array<int, 4> foundation_count = {};
        };

        void set_move_hint(klondike_hint_service::hint_result& result, const bn::string<48>& text,
                           const pile_ref& selection, int tableau_pick_depth_from_top, const pile_ref& move_from,
                           const pile_ref& move_to)
        {
            result.text = text;
            result.has_selection = true;
            result.selection = selection;
            result.tableau_pick_depth_from_top = tableau_pick_depth_from_top;
            result.has_move = true;
            result.move_from = move_from;
            result.move_to = move_to;
        }

        void set_waste_to_foundation_hint(klondike_hint_service::hint_result& result, int foundation_index)
        {
            bn::string<48> text("HINT: WASTE -> F");
            text += bn::to_string<1>(foundation_index + 1);
            set_move_hint(result, text, { pile_kind::waste, 0 }, 0, { pile_kind::waste, 0 },
                          { pile_kind::foundation, foundation_index });
        }

        void set_waste_to_tableau_hint(klondike_hint_service::hint_result& result, int tableau_index)
        {
            bn::string<48> text("HINT: WASTE -> T");
            text += bn::to_string<1>(tableau_index + 1);
            set_move_hint(result, text, { pile_kind::waste, 0 }, 0, { pile_kind::waste, 0 },
                          { pile_kind::tableau, tableau_index });
        }

        void set_tableau_to_foundation_hint(klondike_hint_service::hint_result& result, int tableau_index,
                                            int foundation_index)
        {
            bn::string<48> text("HINT: T");
            text += bn::to_string<1>(tableau_index + 1);
            text += " -> F";
            text += bn::to_string<1>(foundation_index + 1);
            set_move_hint(result, text, { pile_kind::tableau, tableau_index }, 0,
                          { pile_kind::tableau, tableau_index },
                          { pile_kind::foundation, foundation_index });
        }

        void set_tableau_to_tableau_hint(klondike_hint_service::hint_result& result, int from_index, int from_face_up,
                                         int pickup_index, int to_index)
        {
            bn::string<48> text("HINT: T");
            text += bn::to_string<1>(from_index + 1);
            text += " -> T";
            text += bn::to_string<1>(to_index + 1);
            set_move_hint(result, text, { pile_kind::tableau, from_index },
                          from_face_up - 1 - pickup_index, { pile_kind::tableau, from_index },
                          { pile_kind::tableau, to_index });
        }

        [[nodiscard]] bool can_place_on_foundation(const card& moving_card, const card& top_card, bool has_top_card)
        {
            if(! has_top_card)
            {
                return moving_card.rank == 1;
            }

            return moving_card.card_suit == top_card.card_suit && moving_card.rank == (top_card.rank + 1);
        }

        [[nodiscard]] bool can_place_on_tableau(const card& moving_card, const card& top_card, bool has_top_card)
        {
            if(! has_top_card)
            {
                return moving_card.rank == 13;
            }

            return moving_card.rank == (top_card.rank - 1) && moving_card.is_red() != top_card.is_red();
        }

        [[nodiscard]] bool can_place_on_tableau(const card& moving_card, const tableau_hint_state& tableau)
        {
            if(tableau.face_up.empty())
            {
                return moving_card.rank == 13;
            }

            const card& top_card = tableau.face_up.back();
            return moving_card.rank == (top_card.rank - 1) && moving_card.is_red() != top_card.is_red();
        }

        [[nodiscard]] bool can_move_to_any_foundation(const hint_state& state, const card& moving_card)
        {
            for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
            {
                if(can_place_on_foundation(moving_card, state.foundation_top[foundation_index],
                                           state.foundation_has_top[foundation_index]))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool can_move_to_any_tableau(const hint_state& state, const card& moving_card, int from_index)
        {
            for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
            {
                if(tableau_index != from_index &&
                   can_place_on_tableau(moving_card, state.tableaus[tableau_index]))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] hint_state build_hint_state(const klondike_game& game)
        {
            hint_state state;

            for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
            {
                state.foundation_count[foundation_index] = game.foundation_size(foundation_index);
                state.foundation_has_top[foundation_index] = game.foundation_top(foundation_index, state.foundation_top[foundation_index]);
            }

            for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
            {
                auto& tableau = state.tableaus[tableau_index];
                tableau.face_down_count = game.tableau_face_down_size(tableau_index);
                const int face_up_count = game.tableau_face_up_size(tableau_index);
                for(int up_index = 0; up_index < face_up_count; ++up_index)
                {
                    card value;
                    if(game.tableau_face_up_card(tableau_index, up_index, value))
                    {
                        tableau.face_up.push_back(value);
                    }
                }
            }

            return state;
        }

        [[nodiscard]] int total_face_down_count(const hint_state& state)
        {
            int total = 0;
            for(const auto& tableau : state.tableaus)
            {
                total += tableau.face_down_count;
            }
            return total;
        }

        [[nodiscard]] int total_foundation_count(const hint_state& state)
        {
            int total = 0;
            for(int count : state.foundation_count)
            {
                total += count;
            }
            return total;
        }

        [[nodiscard]] bool has_progress_vs_baseline(const hint_state& state, int baseline_face_down,
                                                    int baseline_foundation)
        {
            return total_face_down_count(state) < baseline_face_down ||
                   total_foundation_count(state) > baseline_foundation;
        }

        [[nodiscard]] unsigned hash_state(const hint_state& state)
        {
            unsigned hash = 0x243F6A88u;
            for(const auto& tableau : state.tableaus)
            {
                hash ^= unsigned(tableau.face_down_count) + 0x9E3779B9u + (hash << 6) + (hash >> 2);
                for(const card& value : tableau.face_up)
                {
                    const unsigned encoded = unsigned(value.rank) | (unsigned(value.card_suit) << 4);
                    hash ^= encoded + 0x9E3779B9u + (hash << 6) + (hash >> 2);
                }
                hash ^= 0x5B5B5B5Bu;
            }

            for(int index = 0; index < 4; ++index)
            {
                hash ^= unsigned(state.foundation_count[index]) + 0x9E3779B9u + (hash << 6) + (hash >> 2);
                if(state.foundation_has_top[index])
                {
                    const card& top = state.foundation_top[index];
                    const unsigned encoded = unsigned(top.rank) | (unsigned(top.card_suit) << 4);
                    hash ^= encoded + 0x9E3779B9u + (hash << 6) + (hash >> 2);
                }
            }

            return hash == 0 ? 1u : hash;
        }

        [[nodiscard]] bool simulate_tableau_move(const hint_state& state, int from_index, int pickup_index, int to_index,
                                                 hint_state& out_state)
        {
            if(from_index == to_index)
            {
                return false;
            }

            const auto& from = state.tableaus[from_index];
            const int from_face_up = from.face_up.size();
            if(pickup_index < 0 || pickup_index >= from_face_up)
            {
                return false;
            }

            const card moving_base = from.face_up[pickup_index];
            const auto& to = state.tableaus[to_index];
            if(! can_place_on_tableau(moving_base, to))
            {
                return false;
            }

            out_state = state;
            auto& mutable_from = out_state.tableaus[from_index];
            auto& mutable_to = out_state.tableaus[to_index];

            for(int index = pickup_index; index < from_face_up; ++index)
            {
                mutable_to.face_up.push_back(mutable_from.face_up[index]);
            }

            while(mutable_from.face_up.size() > pickup_index)
            {
                mutable_from.face_up.pop_back();
            }

            if(mutable_from.face_up.empty() && mutable_from.face_down_count > 0)
            {
                // Unknown revealed card; we only need count changes for hint progress checks.
                --mutable_from.face_down_count;
            }

            return true;
        }

        [[nodiscard]] bool simulate_tableau_to_foundation_move(const hint_state& state, int from_index,
                                                               hint_state& out_state)
        {
            const auto& from = state.tableaus[from_index];
            if(from.face_up.empty())
            {
                return false;
            }

            const card moving_card = from.face_up.back();
            for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
            {
                if(can_place_on_foundation(moving_card, state.foundation_top[foundation_index],
                                           state.foundation_has_top[foundation_index]))
                {
                    out_state = state;
                    auto& mutable_from = out_state.tableaus[from_index];
                    mutable_from.face_up.pop_back();
                    if(mutable_from.face_up.empty() && mutable_from.face_down_count > 0)
                    {
                        --mutable_from.face_down_count;
                    }

                    out_state.foundation_count[foundation_index] += 1;
                    out_state.foundation_has_top[foundation_index] = true;
                    out_state.foundation_top[foundation_index] = moving_card;
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool has_progress_with_lookahead(const hint_state& state, int depth, int baseline_face_down,
                                                       int baseline_foundation, unsigned* visited_hashes,
                                                       int& visited_count)
        {
            if(has_progress_vs_baseline(state, baseline_face_down, baseline_foundation))
            {
                return true;
            }

            if(depth <= 0)
            {
                return false;
            }

            const unsigned state_hash = hash_state(state);
            for(int index = 0; index < visited_count; ++index)
            {
                if(visited_hashes[index] == state_hash)
                {
                    return false;
                }
            }

            if(visited_count >= 64)
            {
                return false;
            }

            visited_hashes[visited_count++] = state_hash;

            for(int from_index = 0; from_index < 7; ++from_index)
            {
                hint_state next_state;
                if(simulate_tableau_to_foundation_move(state, from_index, next_state) &&
                   has_progress_with_lookahead(next_state, depth - 1, baseline_face_down, baseline_foundation,
                                               visited_hashes, visited_count))
                {
                    return true;
                }
            }

            for(int from_index = 0; from_index < 7; ++from_index)
            {
                const int from_face_up = state.tableaus[from_index].face_up.size();
                for(int pickup_index = 0; pickup_index < from_face_up; ++pickup_index)
                {
                    for(int to_index = 0; to_index < 7; ++to_index)
                    {
                        hint_state next_state;
                        if(simulate_tableau_move(state, from_index, pickup_index, to_index, next_state) &&
                           has_progress_with_lookahead(next_state, depth - 1, baseline_face_down, baseline_foundation,
                                                       visited_hashes, visited_count))
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool try_waste_to_foundation_hint(const klondike_game& game,
                                                        klondike_hint_service::hint_result& result,
                                                        const card& waste_top, card& top_card)
        {
            for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
            {
                const bool has_top = game.foundation_top(foundation_index, top_card);
                if(can_place_on_foundation(waste_top, top_card, has_top))
                {
                    set_waste_to_foundation_hint(result, foundation_index);
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool try_waste_to_tableau_hint(const klondike_game& game, klondike_hint_service::hint_result& result,
                                                     const card& waste_top, card& top_card)
        {
            for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
            {
                const int face_up_count = game.tableau_face_up_size(tableau_index);
                const bool has_top = face_up_count > 0 &&
                                     game.tableau_face_up_card(tableau_index, face_up_count - 1, top_card);
                if(can_place_on_tableau(waste_top, top_card, has_top))
                {
                    set_waste_to_tableau_hint(result, tableau_index);
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool try_tableau_to_foundation_hint(const klondike_game& game,
                                                          klondike_hint_service::hint_result& result,
                                                          const int* tableau_order, card& moving_card, card& top_card)
        {
            for(int order_index = 0; order_index < 7; ++order_index)
            {
                const int tableau_index = tableau_order[order_index];
                const int face_up_count = game.tableau_face_up_size(tableau_index);
                if(face_up_count <= 0 || ! game.tableau_face_up_card(tableau_index, face_up_count - 1, moving_card))
                {
                    continue;
                }

                for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
                {
                    const bool has_top = game.foundation_top(foundation_index, top_card);
                    if(can_place_on_foundation(moving_card, top_card, has_top))
                    {
                        set_tableau_to_foundation_hint(result, tableau_index, foundation_index);
                        return true;
                    }
                }
            }

            return false;
        }

        void build_tableau_priority_order(const klondike_game& game, int* out_order)
        {
            for(int index = 0; index < 7; ++index)
            {
                out_order[index] = index;
            }

            // Prefer source columns with more unknown cards.
            for(int start = 0; start < 6; ++start)
            {
                int best = start;
                int best_hidden = game.tableau_face_down_size(out_order[start]);
                for(int probe = start + 1; probe < 7; ++probe)
                {
                    const int probe_hidden = game.tableau_face_down_size(out_order[probe]);
                    if(probe_hidden > best_hidden)
                    {
                        best = probe;
                        best_hidden = probe_hidden;
                    }
                }

                if(best != start)
                {
                    const int tmp = out_order[start];
                    out_order[start] = out_order[best];
                    out_order[best] = tmp;
                }
            }
        }
    }

    klondike_hint_service::hint_result klondike_hint_service::build_hint(const klondike_game& game) const
    {
        hint_result result;
        card moving_card;
        card top_card;
        int tableau_order[7] = {};
        build_tableau_priority_order(game, tableau_order);
        const bool has_waste_top = game.waste_card_from_top(0, moving_card);

        if(has_waste_top && try_waste_to_foundation_hint(game, result, moving_card, top_card))
        {
            return result;
        }

        if(has_waste_top && try_waste_to_tableau_hint(game, result, moving_card, top_card))
        {
            return result;
        }

        if(try_tableau_to_foundation_hint(game, result, tableau_order, moving_card, top_card))
        {
            return result;
        }

        const hint_state current_state = build_hint_state(game);
        const int baseline_face_down = total_face_down_count(current_state);
        const int baseline_foundation = total_foundation_count(current_state);
        for(int order_index = 0; order_index < 7; ++order_index)
        {
            const int from_index = tableau_order[order_index];
            const int from_face_up = game.tableau_face_up_size(from_index);
            if(from_face_up <= 0)
            {
                continue;
            }

            for(int pickup_index = 0; pickup_index < from_face_up; ++pickup_index)
            {
                if(! game.tableau_face_up_card(from_index, pickup_index, moving_card))
                {
                    continue;
                }

                for(int to_index = 0; to_index < 7; ++to_index)
                {
                    if(to_index == from_index)
                    {
                        continue;
                    }

                    const int to_face_up = game.tableau_face_up_size(to_index);
                    const bool has_top = to_face_up > 0 && game.tableau_face_up_card(to_index, to_face_up - 1, top_card);

                    if(! can_place_on_tableau(moving_card, top_card, has_top))
                    {
                        continue;
                    }

                    bool allow_hint = false;
                    hint_state next_state;
                    if(simulate_tableau_move(current_state, from_index, pickup_index, to_index, next_state))
                    {
                        const bool reveals_hidden =
                                next_state.tableaus[from_index].face_down_count <
                                current_state.tableaus[from_index].face_down_count;

                        bool exposes_usable_card = false;
                        const auto& from_after_move = next_state.tableaus[from_index];
                        if(! from_after_move.face_up.empty())
                        {
                            const card& exposed_card = from_after_move.face_up.back();
                            exposes_usable_card =
                                    can_move_to_any_foundation(next_state, exposed_card) ||
                                    can_move_to_any_tableau(next_state, exposed_card, from_index);
                        }

                        if(reveals_hidden || exposes_usable_card)
                        {
                            unsigned visited_hashes[64] = {};
                            int visited_count = 0;
                            allow_hint = has_progress_with_lookahead(next_state, tableau_progress_lookahead_depth,
                                                                     baseline_face_down, baseline_foundation,
                                                                     visited_hashes, visited_count);
                        }
                    }

                    if(allow_hint)
                    {
                        set_tableau_to_tableau_hint(result, from_index, from_face_up, pickup_index, to_index);
                        return result;
                    }
                }

            }
        }

        if(game.stock_size() > 0)
        {
            result.text = "HINT: DRAW FROM STOCK";
            result.has_selection = true;
            result.selection = { pile_kind::stock, 0 };
            result.has_move = false;
            return result;
        }

        if(game.waste_size() > 0)
        {
            bn::string<48> text("HINT: RECYCLE STOCK");
            set_move_hint(result, text, { pile_kind::stock, 0 }, 0, { pile_kind::stock, 0 },
                          { pile_kind::waste, 0 });
            return result;
        }

        result.text = "HINT: NO OBVIOUS MOVE";
        return result;
    }

}
