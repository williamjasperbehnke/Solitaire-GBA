#ifndef SOLITAIRE_GAME_AUDIO_H
#define SOLITAIRE_GAME_AUDIO_H

namespace solitaire
{

    class game_audio
    {
    public:
        void update();

        void on_deal_started();
        void on_deal_animation_frame(int deal_animation_frame, int frames_per_card, int total_cards);
        void on_selection_changed();
        void on_return_to_prompt();
        void on_cancel_pressed(bool had_held_cards);
        void on_place_attempt(bool success);
        void on_draw_from_stock(bool success);
        void on_pick_attempt(bool success);
        void on_win_state(bool has_won);

    private:
        int _last_deal_step = -1;
        int _cursor_cooldown_frames = 0;
        bool _played_win_sfx = false;
    };

}

#endif
