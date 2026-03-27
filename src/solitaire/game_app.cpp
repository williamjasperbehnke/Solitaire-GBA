#include "solitaire/game_app.h"

#include "bn_keypad.h"
#include "bn_regular_bg_items_table_bg.h"
#include "bn_sprite_items_slot_highlight.h"
#include "bn_sprite_items_waste_highlight.h"
#include "bn_string.h"
#include "bn_timers.h"

#include "bn_sprite_palette_items_deck_shared_palette.h"
#include "common_variable_8x16_sprite_font.h"
#include "solitaire/card_sprite_registry.h"

namespace solitaire
{
    namespace
    {
        constexpr int stock_selection_index = 0;
        constexpr int waste_selection_index = 1;
        constexpr int first_foundation_selection_index = 2;
        constexpr int last_foundation_selection_index = 5;
        constexpr int first_tableau_selection_index = 6;
        constexpr int last_selection_index = 12;

        constexpr int hud_x = -116;
        constexpr int hud_y = -71;
        constexpr int status_y = 71;
        constexpr int held_cards_y = 52;

        constexpr int top_row_y = -42;
        constexpr int tableau_base_y = -2;

        constexpr int stock_x = -96;
        constexpr int waste_x = -64;
        constexpr int waste_selected_with_cards_x = -48;
        constexpr int foundation_base_x = 0;
        constexpr int tableau_base_x = -96;

        constexpr int pile_x_step = 32;
        constexpr int waste_preview_step = 8;
        constexpr int tableau_face_down_step = 4;
        constexpr int tableau_face_up_step = 9;

        constexpr int held_cards_x = -104;
        constexpr int held_cards_step = 8;

        constexpr int waste_preview_count = 3;

        [[nodiscard]] bn::string<8> selected_label(int selected_index)
        {
            bn::string<8> output;
            if(selected_index == stock_selection_index)
            {
                output = "STK";
            }
            else if(selected_index == waste_selection_index)
            {
                output = "WST";
            }
            else if(selected_index <= last_foundation_selection_index)
            {
                output = "F";
                output += bn::to_string<1>(selected_index - 1);
            }
            else
            {
                output = "T";
                output += bn::to_string<1>(selected_index - 5);
            }
            return output;
        }
    }

    game_app::game_app() :
        _table_bg(bn::regular_bg_items::table_bg.create_bg(0, 0)),
        _text_generator(common::variable_8x16_sprite_font)
    {
        _table_bg.set_priority(3);
        _text_generator.set_left_alignment();
        _text_generator.set_bg_priority(0);
        _text_generator.set_z_order(-100);
    }

    void game_app::update()
    {
        if(! _game.has_won())
        {
            _elapsed_ticks = _time_timer.elapsed_ticks();
        }

        _update_input();
        _render();
    }

    pile_ref game_app::_selected_pile() const
    {
        if(_selected_index == stock_selection_index)
        {
            return pile_ref { pile_kind::stock, 0 };
        }

        if(_selected_index == waste_selection_index)
        {
            return pile_ref { pile_kind::waste, 0 };
        }

        if(_selected_index >= first_foundation_selection_index && _selected_index <= last_foundation_selection_index)
        {
            return pile_ref { pile_kind::foundation, _selected_index - first_foundation_selection_index };
        }

        return pile_ref { pile_kind::tableau, _selected_index - first_tableau_selection_index };
    }

    void game_app::_update_input()
    {
        if(bn::keypad::left_pressed())
        {
            --_selected_index;
            if(_selected_index < 0)
            {
                _selected_index = last_selection_index;
            }
            _tableau_pick_depth_from_top = 0;
        }

        if(bn::keypad::right_pressed())
        {
            ++_selected_index;
            if(_selected_index > last_selection_index)
            {
                _selected_index = 0;
            }
            _tableau_pick_depth_from_top = 0;
        }

        if(! _game.has_held_card() && _selected_index >= first_tableau_selection_index)
        {
            const int tableau_index = _selected_index - first_tableau_selection_index;
            const int face_up_count = _game.tableau_face_up_size(tableau_index);
            const int max_depth = face_up_count > 0 ? face_up_count - 1 : 0;
            if(_tableau_pick_depth_from_top > max_depth)
            {
                _tableau_pick_depth_from_top = max_depth;
            }

            if(bn::keypad::up_pressed() && _tableau_pick_depth_from_top < max_depth)
            {
                ++_tableau_pick_depth_from_top;
            }

            if(bn::keypad::down_pressed() && _tableau_pick_depth_from_top > 0)
            {
                --_tableau_pick_depth_from_top;
            }
        }

        if(bn::keypad::start_pressed())
        {
            _game.reset();
            _tableau_pick_depth_from_top = 0;
            _time_timer.restart();
            _elapsed_ticks = 0;
            _moves_count = 0;
            return;
        }

        if(bn::keypad::b_pressed())
        {
            _game.cancel_held();
            return;
        }

        if(! bn::keypad::a_pressed())
        {
            return;
        }

        const pile_ref selected = _selected_pile();

        if(_game.has_held_card())
        {
            const pile_ref held_from = _game.held_from();
            if(_game.try_place(selected))
            {
                if(selected.kind != held_from.kind || selected.index != held_from.index)
                {
                    ++_moves_count;
                }
            }
            return;
        }

        if(selected.kind == pile_kind::stock)
        {
            if(_game.draw_from_stock())
            {
                ++_moves_count;
            }
            return;
        }

        if(selected.kind == pile_kind::tableau)
        {
            if(_game.try_pick(selected, _tableau_pick_depth_from_top))
            {
                _tableau_pick_depth_from_top = 0;
            }
            return;
        }

        (void) _game.try_pick(selected, 0);
    }

    void game_app::_render()
    {
        _text_sprites.clear();
        _card_sprites.clear();

        bn::string<48> line;
        card top_card;

        line = _time_moves_text(_elapsed_ticks, _moves_count);
        line += "  SEL ";
        line += selected_label(_selected_index);
        _text_generator.generate(hud_x, hud_y, line, _text_sprites);

        // Stock.
        if(_game.stock_size() > 0)
        {
            _draw_card_back_sprite(stock_x, top_row_y);
        }

        // Waste: show top 3 cards.
        for(int waste_offset = waste_preview_count - 1; waste_offset >= 0; --waste_offset)
        {
            if(_game.waste_card_from_top(waste_offset, top_card))
            {
                const int x = waste_x + (((waste_preview_count - 1) - waste_offset) * waste_preview_step);
                _draw_card_sprite(top_card, x, top_row_y);
            }
        }

        // Foundations.
        for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
        {
            const int x = foundation_base_x + (foundation_index * pile_x_step);
            if(_game.foundation_top(foundation_index, top_card))
            {
                _draw_card_sprite(top_card, x, top_row_y);
            }
        }

        // Tableau: draw backs then all face-up cards.
        for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
        {
            const int x = tableau_base_x + (tableau_index * pile_x_step);
            const int face_down_count = _game.tableau_face_down_size(tableau_index);
            const int face_up_count = _game.tableau_face_up_size(tableau_index);

            for(int down_index = 0; down_index < face_down_count; ++down_index)
            {
                const int y = tableau_base_y + (down_index * tableau_face_down_step);
                _draw_card_back_sprite(x, y);
            }

            for(int up_index = 0; up_index < face_up_count; ++up_index)
            {
                if(_game.tableau_face_up_card(tableau_index, up_index, top_card))
                {
                    const int y = tableau_base_y + (face_down_count * tableau_face_down_step) +
                                  (up_index * tableau_face_up_step);
                    _draw_card_sprite(top_card, x, y);
                }
            }
        }

        int marker_x = stock_x;
        int marker_y = top_row_y;
        if(_selected_index == stock_selection_index)
        {
            marker_x = stock_x;
            marker_y = top_row_y;
        }
        else if(_selected_index == waste_selection_index)
        {
            marker_x = _game.waste_size() > 0 ? waste_selected_with_cards_x : waste_x;
            marker_y = top_row_y;
        }
        else if(_selected_index <= last_foundation_selection_index)
        {
            marker_x = foundation_base_x + ((_selected_index - first_foundation_selection_index) * pile_x_step);
            marker_y = top_row_y;
        }
        else
        {
            const int tableau_index = _selected_index - first_tableau_selection_index;
            const int face_down_count = _game.tableau_face_down_size(tableau_index);
            const int face_up_count = _game.tableau_face_up_size(tableau_index);
            marker_x = tableau_base_x + (tableau_index * pile_x_step);

            if(face_up_count > 0)
            {
                int depth = _game.has_held_card() ? 0 : _tableau_pick_depth_from_top;
                if(depth > face_up_count - 1)
                {
                    depth = face_up_count - 1;
                }

                marker_y = tableau_base_y + (face_down_count * tableau_face_down_step) +
                           ((face_up_count - 1 - depth) * tableau_face_up_step);
            }
            else if(face_down_count > 0)
            {
                marker_y = tableau_base_y + ((face_down_count - 1) * tableau_face_down_step);
            }
            else
            {
                marker_y = tableau_base_y;
            }
        }

        const bool use_waste_style = _selected_index == waste_selection_index && _game.waste_size() == 0;
        _draw_selection_highlight(marker_x, marker_y, use_waste_style);

        if(_game.has_held_card())
        {
            const int held_cards_count = _game.held_cards_count();
            for(int index = 0; index < held_cards_count; ++index)
            {
                _draw_card_sprite(_game.held_card(index), held_cards_x + (index * held_cards_step), held_cards_y);
            }
            _text_generator.generate(hud_x, status_y, "HOLD", _text_sprites);
        }
        else if(_game.has_won())
        {
            _text_generator.generate(hud_x, status_y, "YOU WIN! START FOR NEW DEAL", _text_sprites);
        }
    }

    void game_app::_draw_card_sprite(const card& value, int x, int y)
    {
        bn::sprite_ptr sprite = card_face_item(value).create_sprite(x, y);
        sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
        sprite.set_bg_priority(1);
        _card_sprites.push_back(bn::move(sprite));
    }

    void game_app::_draw_card_back_sprite(int x, int y)
    {
        bn::sprite_ptr sprite = card_back_item().create_sprite(x, y);
        sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
        sprite.set_bg_priority(1);
        _card_sprites.push_back(bn::move(sprite));
    }

    void game_app::_draw_selection_highlight(int x, int y, bool use_waste_style)
    {
        bn::sprite_ptr sprite = use_waste_style ?
                                        bn::sprite_items::waste_highlight.create_sprite(x, y) :
                                        bn::sprite_items::slot_highlight.create_sprite(x, y);
        sprite.set_bg_priority(0);
        _card_sprites.push_back(bn::move(sprite));
    }

    bn::string<32> game_app::_time_moves_text(int elapsed_ticks, int moves_count)
    {
        const int elapsed_seconds = elapsed_ticks / bn::timers::ticks_per_second();
        const int minutes = elapsed_seconds / 60;
        const int seconds = elapsed_seconds % 60;

        bn::string<32> output("TIME ");
        output += bn::to_string<3>(minutes);
        output += ":";
        if(seconds < 10)
        {
            output += "0";
        }
        output += bn::to_string<2>(seconds);
        output += "  MOVES ";
        output += bn::to_string<4>(moves_count);
        return output;
    }

}
