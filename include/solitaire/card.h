#ifndef SOLITAIRE_CARD_H
#define SOLITAIRE_CARD_H

namespace solitaire
{

    enum class suit
    {
        clubs,
        diamonds,
        hearts,
        spades,
    };

    struct card
    {
        int rank = 1;  // 1 = Ace, 13 = King
        suit card_suit = suit::clubs;

        [[nodiscard]] bool is_red() const
        {
            return card_suit == suit::diamonds || card_suit == suit::hearts;
        }
    };

}

#endif
