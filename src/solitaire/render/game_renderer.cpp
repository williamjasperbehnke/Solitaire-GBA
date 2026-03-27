#include "solitaire/render/game_renderer.h"

#include "bn_regular_bg_items_slot_highlight_bg.h"
#include "bn_regular_bg_items_table_bg.h"
#include "bn_regular_bg_items_waste_highlight_bg.h"
#include "bn_sprite_tiles_items_held_panel_parts.h"
#include "bn_string.h"
#include "bn_timers.h"

#include "bn_sprite_palette_items_deck_shared_palette.h"
#include "common_variable_8x16_sprite_font.h"
#include "solitaire/core/card_sprite_registry.h"
#include "solitaire/render/held_panel_layout.h"
#include "solitaire/core/table_layout.h"

namespace solitaire
{
    namespace
    {
        constexpr int held_panel_left_top_graphics_index = 0;
        constexpr int held_panel_middle_top_graphics_index = 1;
        constexpr int held_panel_right_top_graphics_index = 2;
        constexpr int held_panel_left_bottom_graphics_index = 3;
        constexpr int held_panel_middle_bottom_graphics_index = 4;
        constexpr int held_panel_right_bottom_graphics_index = 5;

        constexpr int hud_x = -116;
        constexpr int hud_y = -71;
        constexpr int status_y = 71;

        constexpr int held_cards_y = 52;
        constexpr int held_cards_step = 8;

        constexpr int waste_preview_count = 3;
        constexpr int waste_preview_step = 8;
        constexpr int tableau_face_down_step = 4;

        constexpr int selected_card_lift_x = 4;
        constexpr int selected_card_lift_y = -4;
        constexpr int dealing_card_lift_frames = 3;
        constexpr int deal_animation_cards = 28;

        constexpr int panel_tile_size = 16;
        constexpr int panel_tile_half = panel_tile_size / 2;

        constexpr bn::sprite_item held_panel_parts_item(
                bn::sprite_tiles_items::held_panel_parts_shape_size,
                bn::sprite_tiles_items::held_panel_parts,
                bn::sprite_palette_items::deck_shared_palette);

        [[nodiscard]] constexpr int tableau_face_up_step_for_count(int face_up_count)
        {
            if(face_up_count <= 10)
            {
                return 7;
            }
            if(face_up_count <= 14)
            {
                return 5;
            }
            if(face_up_count <= 18)
            {
                return 4;
            }
            return 3;
        }

        [[nodiscard]] constexpr int selected_tableau_depth_from_top(bool has_held_card, int pick_depth_from_top,
                                                                    int face_up_count)
        {
            if(face_up_count <= 0)
            {
                return 0;
            }

            int depth = has_held_card ? 0 : pick_depth_from_top;
            if(depth > face_up_count - 1)
            {
                depth = face_up_count - 1;
            }
            return depth;
        }

        constexpr void deal_target_for_step(int step, int& column, int& row)
        {
            int remaining = step;

            for(row = 0; row < 7; ++row)
            {
                const int row_cards = 7 - row;
                if(remaining < row_cards)
                {
                    column = row + remaining;
                    return;
                }

                remaining -= row_cards;
            }

            column = 6;
            row = 6;
        }
    }

    game_renderer::game_renderer() :
        _table_bg(bn::regular_bg_items::table_bg.create_bg(0, 0)),
        _slot_highlight_bg(bn::regular_bg_items::slot_highlight_bg.create_bg(0, 0)),
        _waste_highlight_bg(bn::regular_bg_items::waste_highlight_bg.create_bg(0, 0)),
        _text_generator(common::variable_8x16_sprite_font)
    {
        _table_bg.set_priority(3);
        _slot_highlight_bg.set_priority(2);
        _waste_highlight_bg.set_priority(2);
        _slot_highlight_bg.set_visible(false);
        _waste_highlight_bg.set_visible(false);

        _text_generator.set_left_alignment();
        _text_generator.set_bg_priority(0);
        _text_generator.set_z_order(-100);
    }

    void game_renderer::render(const klondike_game& game, const table_selection& selection, int elapsed_ticks,
                               int moves_count, bool show_press_start_prompt, bool show_deal_animation,
                               int deal_animation_frame, unsigned animation_frame)
    {
        _text_sprites.clear();
        _card_sprites.clear();

        bn::string<48> line = _time_moves_text(elapsed_ticks, moves_count);
        if(! show_press_start_prompt && ! show_deal_animation)
        {
            line += "  SEL ";
            line += selection.label();
        }
        _text_generator.generate(hud_x, hud_y, line, _text_sprites);

        if(show_press_start_prompt)
        {
            _slot_highlight_bg.set_visible(false);
            _waste_highlight_bg.set_visible(false);
            _render_press_start_prompt(animation_frame);
            return;
        }

        if(show_deal_animation)
        {
            _slot_highlight_bg.set_visible(false);
            _waste_highlight_bg.set_visible(false);
            _render_deal_animation(game, deal_animation_frame);
            return;
        }

        const table_selection::highlight_state highlight = selection.highlight();
        _update_selection_highlight(highlight.x, highlight.y, highlight.use_waste_style);

        card top_card;
        const bool lift_selected_card = ! game.has_held_card();
        _render_top_row(game, selection, lift_selected_card, top_card);
        _render_tableau(game, selection, lift_selected_card, top_card);
        _render_held_cards(game);
        _render_status_message(game);
    }

    void game_renderer::_render_press_start_prompt(unsigned animation_frame)
    {
        const int bob_offset = ((animation_frame / 24) % 2) ? -1 : 1;
        const bool show_indicator = ((animation_frame / 40) % 2) == 0;
        constexpr int base_x = -92;
        const int y = 28 + bob_offset;

        _text_generator.set_left_alignment();
        _text_generator.generate(base_x, y, "PRESS START TO DEAL", _text_sprites);
        if(show_indicator)
        {
            _text_generator.generate(base_x + 168, y, ">>", _text_sprites);
        }
    }

    void game_renderer::_render_deal_animation(const klondike_game& game, int deal_animation_frame)
    {
        int dealt_cards = deal_animation_frame / dealing_card_lift_frames;
        if(dealt_cards > deal_animation_cards)
        {
            dealt_cards = deal_animation_cards;
        }
        const int moving_step = dealt_cards;
        const int step_frame = deal_animation_frame % dealing_card_lift_frames;

        for(int step = 0; step < dealt_cards; ++step)
        {
            int column = 0;
            int row = 0;
            deal_target_for_step(step, column, row);

            const int x = table_layout::tableau_base_x + (column * table_layout::pile_x_step);
            const int y = table_layout::tableau_base_y + (row * tableau_face_down_step);
            if(row == column)
            {
                card face_up_card;
                if(game.tableau_face_up_card(column, 0, face_up_card))
                {
                    _draw_card_sprite(face_up_card, x, y);
                }
            }
            else
            {
                _draw_card_back_sprite(x, y);
            }
        }

        if(moving_step < deal_animation_cards)
        {
            int column = 0;
            int row = 0;
            deal_target_for_step(moving_step, column, row);

            const int source_x = table_layout::stock_x;
            const int source_y = table_layout::top_row_y;
            const int target_x = table_layout::tableau_base_x + (column * table_layout::pile_x_step);
            const int target_y = table_layout::tableau_base_y + (row * tableau_face_down_step);

            const int x = source_x + ((target_x - source_x) * step_frame) / dealing_card_lift_frames;
            const int y = source_y + ((target_y - source_y) * step_frame) / dealing_card_lift_frames;
            _draw_card_back_sprite(x, y);
        }
    }

    void game_renderer::_render_top_row(const klondike_game& game, const table_selection& selection, bool lift_selected_card,
                                        card& top_card)
    {
        if(game.stock_size() > 0)
        {
            int x = table_layout::stock_x;
            int y = table_layout::top_row_y;
            if(lift_selected_card && selection.is_stock_selected())
            {
                x += selected_card_lift_x;
                y += selected_card_lift_y;
            }
            _draw_card_back_sprite(x, y);
        }

        for(int waste_offset = waste_preview_count - 1; waste_offset >= 0; --waste_offset)
        {
            if(game.waste_card_from_top(waste_offset, top_card))
            {
                const int x = table_layout::waste_x + (((waste_preview_count - 1) - waste_offset) * waste_preview_step);
                int draw_x = x;
                int draw_y = table_layout::top_row_y;
                if(lift_selected_card && selection.is_waste_selected() && waste_offset == 0)
                {
                    draw_x += selected_card_lift_x;
                    draw_y += selected_card_lift_y;
                }
                _draw_card_sprite(top_card, draw_x, draw_y);
            }
        }

        for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
        {
            const int x = table_layout::foundation_base_x + (foundation_index * table_layout::pile_x_step);
            if(game.foundation_top(foundation_index, top_card))
            {
                int draw_x = x;
                int draw_y = table_layout::top_row_y;
                if(lift_selected_card && selection.selected_foundation_index() == foundation_index)
                {
                    draw_x += selected_card_lift_x;
                    draw_y += selected_card_lift_y;
                }
                _draw_card_sprite(top_card, draw_x, draw_y);
            }
        }
    }

    void game_renderer::_render_tableau(const klondike_game& game, const table_selection& selection, bool lift_selected_card,
                                        card& top_card)
    {
        const int selected_tableau_index = selection.selected_tableau_index();

        for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
        {
            const int x = table_layout::tableau_base_x + (tableau_index * table_layout::pile_x_step);
            const int face_down_count = game.tableau_face_down_size(tableau_index);
            const int face_up_count = game.tableau_face_up_size(tableau_index);
            const int face_up_step = tableau_face_up_step_for_count(face_up_count);

            const bool selected_tableau = selected_tableau_index == tableau_index;
            const int selected_depth_from_top = selected_tableau_depth_from_top(
                    game.has_held_card(), selection.tableau_pick_depth_from_top(), face_up_count);
            const int selected_up_index = face_up_count - 1 - selected_depth_from_top;

            for(int down_index = 0; down_index < face_down_count; ++down_index)
            {
                const int y = table_layout::tableau_base_y + (down_index * tableau_face_down_step);
                _draw_card_back_sprite(x, y);
            }

            for(int up_index = 0; up_index < face_up_count; ++up_index)
            {
                if(game.tableau_face_up_card(tableau_index, up_index, top_card))
                {
                    int draw_x = x;
                    int draw_y = table_layout::tableau_base_y + (face_down_count * tableau_face_down_step) +
                                 (up_index * face_up_step);
                    if(lift_selected_card && selected_tableau && up_index == selected_up_index)
                    {
                        draw_x += selected_card_lift_x;
                        draw_y += selected_card_lift_y;
                    }
                    _draw_card_sprite(top_card, draw_x, draw_y);
                }
            }
        }
    }

    void game_renderer::_render_held_cards(const klondike_game& game)
    {
        if(! game.has_held_card())
        {
            return;
        }

        const int held_cards_count = game.held_cards_count();
        const int first_card_x = held_panel_layout::first_card_x(held_cards_count, held_cards_step);
        _draw_held_cards_panel(held_cards_count);

        for(int index = 0; index < held_cards_count; ++index)
        {
            _draw_held_card_sprite(game.held_card(index), first_card_x + (index * held_cards_step), held_cards_y);
        }
    }

    void game_renderer::_render_status_message(const klondike_game& game)
    {
        if(game.has_held_card())
        {
            _text_generator.generate(hud_x, status_y, "HOLD", _text_sprites);
        }
        else if(game.has_won())
        {
            _text_generator.generate(hud_x, status_y, "YOU WIN! START FOR NEW DEAL", _text_sprites);
        }
    }

    void game_renderer::_draw_card_sprite(const card& value, int x, int y)
    {
        bn::sprite_ptr sprite = card_face_item(value).create_sprite(x, y);
        sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
        sprite.set_bg_priority(1);
        _card_sprites.push_back(bn::move(sprite));
    }

    void game_renderer::_draw_held_cards_panel(int held_cards_count)
    {
        if(held_cards_count <= 0)
        {
            return;
        }

        const held_panel_layout::geometry panel = held_panel_layout::build(held_cards_count);
        const int top_y = held_cards_y - panel_tile_half;
        const int bottom_y = held_cards_y + panel_tile_half;

        for(int tile_index = 0; tile_index < panel.tile_count; ++tile_index)
        {
            const int x = panel.first_tile_x + (tile_index * panel_tile_size);
            if(tile_index == 0)
            {
                _draw_panel_tile_pair(x, top_y, bottom_y, held_panel_left_top_graphics_index,
                                      held_panel_left_bottom_graphics_index);
            }
            else if(tile_index == panel.tile_count - 1)
            {
                _draw_panel_tile_pair(x, top_y, bottom_y, held_panel_right_top_graphics_index,
                                      held_panel_right_bottom_graphics_index);
            }
            else
            {
                _draw_panel_tile_pair(x, top_y, bottom_y, held_panel_middle_top_graphics_index,
                                      held_panel_middle_bottom_graphics_index);
            }
        }
    }

    void game_renderer::_draw_panel_tile_pair(int x, int top_y, int bottom_y, int top_graphics_index,
                                              int bottom_graphics_index)
    {
        bn::sprite_ptr top_tile = bn::sprite_ptr::create(x, top_y, held_panel_parts_item, top_graphics_index);
        top_tile.set_bg_priority(1);
        top_tile.set_z_order(0);
        _card_sprites.push_back(bn::move(top_tile));

        bn::sprite_ptr bottom_tile = bn::sprite_ptr::create(x, bottom_y, held_panel_parts_item, bottom_graphics_index);
        bottom_tile.set_bg_priority(1);
        bottom_tile.set_z_order(0);
        _card_sprites.push_back(bn::move(bottom_tile));
    }

    void game_renderer::_draw_held_card_sprite(const card& value, int x, int y)
    {
        bn::sprite_ptr sprite = card_face_item(value).create_sprite(x, y);
        sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
        sprite.set_bg_priority(0);
        sprite.set_z_order(-1);
        _card_sprites.push_back(bn::move(sprite));
    }

    void game_renderer::_draw_card_back_sprite(int x, int y)
    {
        bn::sprite_ptr sprite = card_back_item().create_sprite(x, y);
        sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
        sprite.set_bg_priority(1);
        _card_sprites.push_back(bn::move(sprite));
    }

    void game_renderer::_update_selection_highlight(int x, int y, bool use_waste_style)
    {
        if(use_waste_style)
        {
            _slot_highlight_bg.set_visible(false);
            _waste_highlight_bg.set_position(x, y);
            _waste_highlight_bg.set_visible(true);
        }
        else
        {
            _waste_highlight_bg.set_visible(false);
            _slot_highlight_bg.set_position(x, y);
            _slot_highlight_bg.set_visible(true);
        }
    }

    bn::string<32> game_renderer::_time_moves_text(int elapsed_ticks, int moves_count)
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
