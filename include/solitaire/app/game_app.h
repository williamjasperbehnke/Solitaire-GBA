#ifndef SOLITAIRE_GAME_APP_H
#define SOLITAIRE_GAME_APP_H

#include "bn_timer.h"

#include "solitaire/app/hint_overlay.h"
#include "solitaire/input/dpad_repeater.h"
#include "solitaire/render/game_renderer.h"
#include "solitaire/audio/game_audio.h"
#include "solitaire/game/klondike_game.h"
#include "solitaire/game/klondike_hint_service.h"
#include "solitaire/game/run_seed_controller.h"
#include "solitaire/input/table_selection.h"

namespace solitaire
{
    enum class run_phase
    {
        awaiting_deal,
        dealing,
        canceling,
        victory,
        playing
    };

    class game_app
    {
    public:
        game_app();

        void update();

    private:
        void _update_phase();
        void _render_frame();
        void _update_awaiting_deal_phase();
        void _update_dealing_phase();
        void _update_playing_phase();
        void _update_canceling_phase();
        void _update_victory_phase();

        [[nodiscard]] unsigned _auto_seed_entropy() const;
        void _begin_deal();
        void _deal_once();
        void _deal_with_filter();
        void _reset_round_state_for_new_deal();
        [[nodiscard]] int _cancel_animation_cards_count() const;
        void _begin_cancel_sequence();
        void _update_input();
        void _show_hint();
        void _handle_primary_action();

        klondike_game _game;
        table_selection _selection;
        dpad_repeater _dpad_repeater;
        game_renderer _renderer;
        game_audio _audio;
        run_seed_controller _run_seed;
        bn::timer _entropy_timer;
        bn::timer _time_timer;
        int _elapsed_ticks = 0;
        int _moves_count = 0;
        run_phase _phase = run_phase::awaiting_deal;
        int _deal_animation_frame = 0;
        int _cancel_animation_cards = 0;
        int _cancel_animation_total_frames = 0;
        unsigned _runtime_frames = 0;
        klondike_hint_service _hint_service;
        hint_overlay _hint_overlay;
    };

}

#endif
