#ifndef SOLITAIRE_GAME_APP_H
#define SOLITAIRE_GAME_APP_H

#include "bn_timer.h"
#include "bn_vector.h"

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
        struct stock_draw_animation_state
        {
            bool active = false;
            int frame = 0;
            card moving_card = {};
        };

        struct stock_recycle_animation_state
        {
            bool active = false;
            int frame = 0;
        };

        struct card_transfer_animation_state
        {
            bool active = false;
            int frame = 0;
            int source_x = 0;
            int source_y = 0;
            int target_x = 0;
            int target_y = 0;
            int source_stack_step_x = 0;
            int source_stack_step_y = 0;
            int destination_stack_step_x = 0;
            int destination_stack_step_y = 0;
            bn::vector<card, 19> cards;
            bool show_previous_foundation_card = false;
            int previous_foundation_index = 0;
            card previous_foundation_card = {};
        };

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
        void _handle_quick_send_to_foundation();
        [[nodiscard]] int _tableau_destination_step(const pile_ref& pile) const;
        void _begin_transfer_to_target_pile(const bn::vector<card, 19>& cards, int source_x, int source_y,
                                            const pile_ref& target_pile, int moved_cards_count);
        void _begin_transfer_to_held_from_source(const pile_ref& source_pile, int source_x, int source_y);
        void _cancel_held_with_return_animation();
        [[nodiscard]] bool _capture_held_for_animation(pile_ref& held_from, int& source_x, int& source_y,
                                                       bn::vector<card, 19>& cards) const;
        [[nodiscard]] bool _update_active_playing_animations();
        void _reset_all_playing_animations();
        void _begin_stock_draw_animation(const card& moving_card);
        void _complete_stock_draw_animation();
        void _reset_stock_draw_animation();
        [[nodiscard]] game_renderer::stock_draw_animation _renderer_stock_draw_animation() const;
        void _begin_stock_recycle_animation();
        void _complete_stock_recycle_animation();
        void _reset_stock_recycle_animation();
        [[nodiscard]] game_renderer::stock_recycle_animation _renderer_stock_recycle_animation() const;
        void _begin_card_transfer_animation(const card& moving_card, int source_x, int source_y, int target_x,
                                            int target_y, bool show_previous_foundation_card = false,
                                            int previous_foundation_index = 0,
                                            const card& previous_foundation_card = {});
        void _begin_card_transfer_animation(const bn::vector<card, 19>& cards, int source_x, int source_y,
                                            int target_x, int target_y, int source_stack_step_x,
                                            int source_stack_step_y, int destination_stack_step_x,
                                            int destination_stack_step_y,
                                            bool show_previous_foundation_card = false,
                                            int previous_foundation_index = 0,
                                            const card& previous_foundation_card = {});
        void _reset_card_transfer_animation();
        [[nodiscard]] game_renderer::card_transfer_animation _renderer_card_transfer_animation() const;

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
        stock_draw_animation_state _stock_draw_animation;
        stock_recycle_animation_state _stock_recycle_animation;
        card_transfer_animation_state _card_transfer_animation;
        unsigned _runtime_frames = 0;
        klondike_hint_service _hint_service;
        hint_overlay _hint_overlay;
    };

}

#endif
