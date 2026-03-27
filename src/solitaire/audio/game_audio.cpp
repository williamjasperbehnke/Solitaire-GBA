#include "solitaire/audio/game_audio.h"

#include "bn_fixed.h"
#include "bn_sound_items.h"

namespace solitaire
{
    namespace
    {
        constexpr bn::fixed cursor_volume = 0.85;
        constexpr bn::fixed deal_tick_volume = 0.80;
        constexpr bn::fixed pick_volume = 0.92;
        constexpr bn::fixed place_volume = 0.95;
        constexpr bn::fixed invalid_volume = 0.82;
        constexpr bn::fixed win_volume = 1.0;

        constexpr int cursor_repeat_cooldown_frames = 2;

        void play_result_sfx(bool success, const bn::fixed& success_volume, bn::sound_item success_item)
        {
            if(success)
            {
                success_item.play(success_volume);
            }
            else
            {
                bn::sound_items::invalid.play(invalid_volume);
            }
        }
    }

    void game_audio::update()
    {
        if(_cursor_cooldown_frames > 0)
        {
            --_cursor_cooldown_frames;
        }
    }

    void game_audio::on_deal_started()
    {
        _last_deal_step = -1;
        _played_win_sfx = false;
    }

    void game_audio::on_deal_animation_frame(int deal_animation_frame, int frames_per_card, int total_cards)
    {
        const int deal_step = deal_animation_frame / frames_per_card;
        if(deal_step != _last_deal_step && deal_step < total_cards)
        {
            bn::sound_items::deal_tick.play(deal_tick_volume);
            _last_deal_step = deal_step;
        }
    }

    void game_audio::on_selection_changed()
    {
        if(_cursor_cooldown_frames == 0)
        {
            bn::sound_items::cursor_move.play(cursor_volume);
            _cursor_cooldown_frames = cursor_repeat_cooldown_frames;
        }
    }

    void game_audio::on_return_to_prompt()
    {
        bn::sound_items::card_place.play(place_volume);
    }

    void game_audio::on_cancel_pressed(bool had_held_cards)
    {
        if(had_held_cards)
        {
            bn::sound_items::card_place.play(place_volume);
        }
        else
        {
            bn::sound_items::invalid.play(invalid_volume);
        }
    }

    void game_audio::on_place_attempt(bool success)
    {
        play_result_sfx(success, place_volume, bn::sound_items::card_place);
    }

    void game_audio::on_draw_from_stock(bool success)
    {
        play_result_sfx(success, pick_volume, bn::sound_items::card_pick);
    }

    void game_audio::on_pick_attempt(bool success)
    {
        play_result_sfx(success, pick_volume, bn::sound_items::card_pick);
    }

    void game_audio::on_win_state(bool has_won)
    {
        if(has_won && ! _played_win_sfx)
        {
            bn::sound_items::win.play(win_volume);
            _played_win_sfx = true;
        }
    }

}
