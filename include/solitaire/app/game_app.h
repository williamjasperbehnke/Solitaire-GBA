#ifndef SOLITAIRE_GAME_APP_H
#define SOLITAIRE_GAME_APP_H

#include "bn_timer.h"

#include "solitaire/input/dpad_repeater.h"
#include "solitaire/render/game_renderer.h"
#include "solitaire/game/klondike_game.h"
#include "solitaire/input/table_selection.h"

namespace solitaire
{

    class game_app
    {
    public:
        game_app();

        void update();

    private:
        void _update_input();
        void _reset_run_state();
        void _handle_primary_action();

        klondike_game _game;
        table_selection _selection;
        dpad_repeater _dpad_repeater;
        game_renderer _renderer;
        bn::timer _time_timer;
        int _elapsed_ticks = 0;
        int _moves_count = 0;
    };

}

#endif
