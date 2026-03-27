#include "solitaire/game/run_seed_controller.h"

namespace solitaire
{

    unsigned run_seed_controller::next_auto_seed(unsigned entropy)
    {
        _seed_counter = _xorshift32(_seed_counter);
        const unsigned mixed_seed = _seed_counter ^ (entropy << 1) ^ 0x9E3779B9u;
        return _normalize_seed(mixed_seed);
    }

    unsigned run_seed_controller::_xorshift32(unsigned value)
    {
        value ^= value << 13;
        value ^= value >> 17;
        value ^= value << 5;
        return value;
    }

    unsigned run_seed_controller::_normalize_seed(unsigned value)
    {
        return value == 0 ? 1u : value;
    }

}
