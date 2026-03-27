#ifndef SOLITAIRE_RUN_SEED_CONTROLLER_H
#define SOLITAIRE_RUN_SEED_CONTROLLER_H

namespace solitaire
{

    class run_seed_controller
    {
    public:
        [[nodiscard]] unsigned next_auto_seed(unsigned entropy);

    private:
        unsigned _seed_counter = 0xC0DE1234u;

        [[nodiscard]] static unsigned _xorshift32(unsigned value);
        [[nodiscard]] static unsigned _normalize_seed(unsigned value);
    };

}

#endif
