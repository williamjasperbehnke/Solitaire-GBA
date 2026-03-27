#ifndef SOLITAIRE_GAME_APP_H
#define SOLITAIRE_GAME_APP_H

#include "bn_timer.h"

#include "solitaire/input/dpad_repeater.h"
#include "solitaire/render/game_renderer.h"
#include "solitaire/game/klondike_game.h"
#include "solitaire/game/run_seed_controller.h"
#include "solitaire/input/table_selection.h"

namespace solitaire
{
    enum class run_phase
    {
        awaiting_deal,
        dealing,
        playing
    };

    class game_app
    {
    public:
        game_app();

        void update();

    private:
        [[nodiscard]] unsigned _auto_seed_entropy() const;
        void _begin_deal();
        void _update_input();
        void _reset_run_state();
        void _handle_primary_action();

        klondike_game _game;
        table_selection _selection;
        dpad_repeater _dpad_repeater;
        game_renderer _renderer;
        run_seed_controller _run_seed;
        bn::timer _entropy_timer;
        bn::timer _time_timer;
        int _elapsed_ticks = 0;
        int _moves_count = 0;
        run_phase _phase = run_phase::awaiting_deal;
        int _deal_animation_frame = 0;
        unsigned _runtime_frames = 0;
    };

}

#endif
