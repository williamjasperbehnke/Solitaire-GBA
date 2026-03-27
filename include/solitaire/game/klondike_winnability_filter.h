#ifndef SOLITAIRE_KLONDIKE_WINNABILITY_FILTER_H
#define SOLITAIRE_KLONDIKE_WINNABILITY_FILTER_H

#include "solitaire/game/klondike_types.h"

namespace solitaire
{

    enum class deal_quality
    {
        unwinnable_or_unknown,
        too_easy,
        acceptable
    };

    class klondike_winnability_filter
    {
    public:
        [[nodiscard]] deal_quality classify_deal(const stock_pile& stock, const waste_pile& waste,
                                                 const foundation_piles& foundations, const tableau_piles& tableaus,
                                                 int easy_win_depth_limit, int hard_search_depth_limit,
                                                 int easy_search_node_budget, int hard_search_node_budget) const;
        [[nodiscard]] bool is_likely_winnable(const stock_pile& stock, const waste_pile& waste,
                                              const foundation_piles& foundations, const tableau_piles& tableaus,
                                              int step_budget) const;
    };

}

#endif
