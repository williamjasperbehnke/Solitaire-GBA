#ifndef SOLITAIRE_PILE_REF_H
#define SOLITAIRE_PILE_REF_H

namespace solitaire
{

    enum class pile_kind
    {
        stock,
        waste,
        foundation,
        tableau,
    };

    struct pile_ref
    {
        pile_kind kind = pile_kind::stock;
        int index = 0;
    };

}

#endif
