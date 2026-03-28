#include "solitaire/render/game_renderer.h"

#include "bn_regular_bg_items_slot_highlight_bg.h"
#include "bn_regular_bg_items_table_bg.h"
#include "bn_regular_bg_items_waste_highlight_bg.h"
#include "bn_sprite_items_hint_slot_cell_highlight.h"
#include "bn_sprite_items_hint_waste_cell_highlight.h"
#include "bn_sprite_palette_items_deck_shared_palette.h"
#include "bn_sprite_tiles_items_held_panel_parts.h"
#include "bn_string.h"
#include "bn_timers.h"

#include "common_variable_8x16_sprite_font.h"
#include "solitaire/core/animation_timing.h"
#include "solitaire/core/card_sprite_registry.h"
#include "solitaire/core/table_layout.h"
#include "solitaire/render/held_panel_layout.h"
#include "solitaire/render/render_constants.h"

namespace solitaire
{
    namespace
    {
        constexpr int waste_preview_count = render_constants::waste_preview_count;
        constexpr int waste_preview_step = render_constants::waste_preview_step;
        constexpr int tableau_face_down_step = render_constants::tableau_face_down_step;

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

        constexpr int selected_card_lift_x = render_constants::selected_card_lift_x;
        constexpr int selected_card_lift_y = render_constants::selected_card_lift_y;
        constexpr int prompt_bob_period_frames = 24;
        constexpr int prompt_blink_period_frames = 40;
        constexpr int prompt_base_x = -92;
        constexpr int prompt_base_y = 28;
        constexpr int prompt_indicator_offset_x = 168;
        constexpr int waste_hint_x_offset = 8;

        constexpr int panel_tile_size = 16;
        constexpr int panel_tile_half = panel_tile_size / 2;

        constexpr bn::sprite_item held_panel_parts_item(
                bn::sprite_tiles_items::held_panel_parts_shape_size,
                bn::sprite_tiles_items::held_panel_parts,
                bn::sprite_palette_items::deck_shared_palette);

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

        [[nodiscard]] constexpr int triangle_wave_offset(unsigned animation_frame, int phase)
        {
            const int period = 32;
            int step = (int(animation_frame) + phase) % period;
            if(step >= period / 2)
            {
                step = period - 1 - step;
            }

            return (step / 4) - 2;
        }

        [[nodiscard]] table_selection::highlight_state highlight_for_pile(const pile_ref& pile)
        {
            table_selection::highlight_state state;

            switch(pile.kind)
            {
                case pile_kind::stock:
                    state.x = table_layout::stock_x;
                    state.y = table_layout::top_row_y;
                    state.use_waste_style = false;
                    break;

                case pile_kind::waste:
                    state.x = table_layout::waste_x;
                    state.y = table_layout::top_row_y;
                    state.use_waste_style = true;
                    break;

                case pile_kind::foundation:
                    state.x = table_layout::foundation_base_x + (pile.index * table_layout::pile_x_step);
                    state.y = table_layout::top_row_y;
                    state.use_waste_style = false;
                    break;

                case pile_kind::tableau:
                    state.x = table_layout::tableau_base_x + (pile.index * table_layout::pile_x_step);
                    state.y = table_layout::tableau_base_y;
                    state.use_waste_style = false;
                    break;
                default:
                    break;
            }

            return state;
        }

        [[nodiscard]] bool hide_for_transfer(const game_renderer::card_transfer_animation* transfer, int x, int y,
                                             const card& value)
        {
            if(! transfer || transfer->cards_count <= 0 || ! transfer->cards)
            {
                return false;
            }

            for(int index = 0; index < transfer->cards_count; ++index)
            {
                const card& moving = transfer->cards[index];
                const int target_x = transfer->target_x + (index * transfer->destination_stack_step_x);
                const int target_y = transfer->target_y + (index * transfer->destination_stack_step_y);
                const int dx = x - target_x;
                const int dy = y - target_y;
                const bool near_target = dx >= -6 && dx <= 6 && dy >= -6 && dy <= 6;
                if(near_target && value.rank == moving.rank && value.card_suit == moving.card_suit)
                {
                    return true;
                }
            }

            return false;
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
                               int deal_animation_frame, bool show_cancel_animation, int cancel_animation_frame,
                               bool show_victory_animation, int victory_animation_frame,
                               const stock_draw_animation* stock_draw,
                               const stock_recycle_animation* stock_recycle,
                               const card_transfer_animation* transfer,
                               unsigned animation_frame,
                               const bn::string<48>* hint_text, const hint_highlight* hint_cells)
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
            _update_hint_highlights(nullptr);
            _render_press_start_prompt(animation_frame);
            return;
        }

        if(show_deal_animation)
        {
            _slot_highlight_bg.set_visible(false);
            _waste_highlight_bg.set_visible(false);
            _update_hint_highlights(nullptr);
            _animation_renderer.render_deal(game, deal_animation_frame, _card_sprites);
            return;
        }

        if(show_cancel_animation)
        {
            _slot_highlight_bg.set_visible(false);
            _waste_highlight_bg.set_visible(false);
            _update_hint_highlights(nullptr);
            _animation_renderer.render_cancel(game, cancel_animation_frame, _card_sprites);
            return;
        }

        if(show_victory_animation)
        {
            _slot_highlight_bg.set_visible(false);
            _waste_highlight_bg.set_visible(false);
            _update_hint_highlights(nullptr);
            _animation_renderer.render_victory(game, victory_animation_frame, _card_sprites);
            if(((victory_animation_frame / 8) % 2) == 0)
            {
                _text_generator.set_center_alignment();
                _text_generator.generate(0, 0, "VICTORY!", _text_sprites);
                _text_generator.set_left_alignment();
            }
            return;
        }

        if(hint_cells && hint_cells->has_move)
        {
            _update_hint_highlights(hint_cells);
        }
        else
        {
            const table_selection::highlight_state highlight = selection.highlight();
            _update_selection_highlight(highlight.x, highlight.y, highlight.use_waste_style);
            _update_hint_highlights(nullptr);
        }

        card top_card;
        const bool show_won_effects = game.has_won();
        const bool lift_selected_card = ! game.has_held_card();
        _render_top_row(game, selection, lift_selected_card, show_won_effects, transfer, animation_frame, top_card);
        _render_tableau(game, selection, lift_selected_card, transfer, top_card);
        const bool hide_held_cards = transfer && game.has_held_card();
        _render_held_cards(game, hide_held_cards);

        if(stock_draw && stock_draw->moving_card)
        {
            _animation_renderer.render_stock_to_waste(*stock_draw->moving_card, stock_draw->frame,
                                                      animation_timing::stock_draw_frames, _card_sprites);
        }
        else if(stock_recycle)
        {
            _animation_renderer.render_waste_to_stock(stock_recycle->frame, animation_timing::stock_recycle_frames,
                                                      _card_sprites);
        }

        if(transfer && transfer->cards_count > 0 && transfer->cards)
        {
            constexpr int trail_frame_stagger = 2;
            for(int index = 0; index < transfer->cards_count; ++index)
            {
                int card_frame = transfer->frame - (index * trail_frame_stagger);
                if(card_frame < 0)
                {
                    card_frame = 0;
                }

                const int source_x = transfer->source_x + (index * transfer->source_stack_step_x);
                const int source_y = transfer->source_y + (index * transfer->source_stack_step_y);
                const int target_x = transfer->target_x + (index * transfer->destination_stack_step_x);
                const int target_y = transfer->target_y + (index * transfer->destination_stack_step_y);

                _animation_renderer.render_card_flight(transfer->cards[index], source_x, source_y, target_x, target_y,
                                                       card_frame, animation_timing::hold_release_frames,
                                                       _card_sprites);
            }
        }

        if(! hide_held_cards || hint_text)
        {
            _render_status_message(game, hint_text);
        }
    }

    void game_renderer::_render_press_start_prompt(unsigned animation_frame)
    {
        const int bob_offset = ((animation_frame / prompt_bob_period_frames) % 2) ? -1 : 1;
        const bool show_indicator = ((animation_frame / prompt_blink_period_frames) % 2) == 0;
        const int y = prompt_base_y + bob_offset;

        _text_generator.set_left_alignment();
        _text_generator.generate(prompt_base_x, y, "PRESS START TO DEAL", _text_sprites);
        if(show_indicator)
        {
            _text_generator.generate(prompt_base_x + prompt_indicator_offset_x, y, ">>", _text_sprites);
        }
    }

    void game_renderer::_render_top_row(const klondike_game& game, const table_selection& selection,
                                        bool lift_selected_card, bool show_victory_animation,
                                        const card_transfer_animation* transfer, unsigned animation_frame, card& top_card)
    {
        if(game.stock_size() > 0)
        {
            int base_x = table_layout::stock_x;
            int base_y = table_layout::top_row_y;
            if(lift_selected_card && selection.is_stock_selected())
            {
                base_x += selected_card_lift_x;
                base_y += selected_card_lift_y;
            }
            _draw_card_back_sprite(base_x, base_y);
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
                if(! hide_for_transfer(transfer, draw_x, draw_y, top_card))
                {
                    _draw_card_sprite(top_card, draw_x, draw_y);
                }
            }
        }

        for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
        {
            const int x = table_layout::foundation_base_x + (foundation_index * table_layout::pile_x_step);
            if(transfer && transfer->show_previous_foundation_card &&
               foundation_index == transfer->previous_foundation_index)
            {
                if(transfer->previous_foundation_card)
                {
                    int draw_x = x;
                    int draw_y = table_layout::top_row_y;
                    if(show_victory_animation)
                    {
                        draw_y += triangle_wave_offset(animation_frame, foundation_index * 5);
                    }
                    if(lift_selected_card && selection.selected_foundation_index() == foundation_index)
                    {
                        draw_x += selected_card_lift_x;
                        draw_y += selected_card_lift_y;
                    }
                    _draw_card_sprite(*transfer->previous_foundation_card, draw_x, draw_y);
                }
                continue;
            }

            if(game.foundation_top(foundation_index, top_card))
            {
                int draw_x = x;
                int draw_y = table_layout::top_row_y;
                if(show_victory_animation)
                {
                    draw_y += triangle_wave_offset(animation_frame, foundation_index * 5);
                }
                if(lift_selected_card && selection.selected_foundation_index() == foundation_index)
                {
                    draw_x += selected_card_lift_x;
                    draw_y += selected_card_lift_y;
                }
                if(! hide_for_transfer(transfer, draw_x, draw_y, top_card))
                {
                    _draw_card_sprite(top_card, draw_x, draw_y);
                }
            }
        }
    }

    void game_renderer::_render_tableau(const klondike_game& game, const table_selection& selection,
                                        bool lift_selected_card, const card_transfer_animation* transfer, card& top_card)
    {
        const int selected_tableau_index = selection.selected_tableau_index();

        for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
        {
            const int x = table_layout::tableau_base_x + (tableau_index * table_layout::pile_x_step);
            const int face_down_count = game.tableau_face_down_size(tableau_index);
            const int face_up_count = game.tableau_face_up_size(tableau_index);
            const int face_up_step = render_constants::tableau_face_up_step_for_count(face_up_count,
                                                                                       face_down_count);

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
                    if(! hide_for_transfer(transfer, draw_x, draw_y, top_card))
                    {
                        _draw_card_sprite(top_card, draw_x, draw_y);
                    }
                }
            }
        }
    }

    void game_renderer::_render_held_cards(const klondike_game& game, bool hide_held_cards)
    {
        if(hide_held_cards || ! game.has_held_card())
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

    void game_renderer::_render_status_message(const klondike_game& game, const bn::string<48>* hint_text)
    {
        if(hint_text)
        {
            _text_generator.generate(hud_x, status_y, *hint_text, _text_sprites);
        }
        else if(game.has_held_card())
        {
            _text_generator.generate(hud_x, status_y, "HOLD", _text_sprites);
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

    void game_renderer::_update_hint_highlights(const hint_highlight* hint_cells)
    {
        if(! hint_cells || ! hint_cells->has_move)
        {
            return;
        }

        const table_selection::highlight_state from = highlight_for_pile(hint_cells->from);
        const table_selection::highlight_state to = highlight_for_pile(hint_cells->to);

        _update_selection_highlight(from.x, from.y, from.use_waste_style);
        _draw_hint_cell_sprite(from.x, from.y, from.use_waste_style);
        if(from.x != to.x || from.y != to.y || from.use_waste_style != to.use_waste_style)
        {
            _draw_hint_cell_sprite(to.x, to.y, to.use_waste_style);
        }
    }

    void game_renderer::_draw_hint_cell_sprite(int x, int y, bool use_waste_style)
    {
        if(use_waste_style)
        {
            x += waste_hint_x_offset;
        }

        bn::sprite_ptr sprite = use_waste_style ? bn::sprite_items::hint_waste_cell_highlight.create_sprite(x, y) :
                                                  bn::sprite_items::hint_slot_cell_highlight.create_sprite(x, y);
        sprite.set_bg_priority(2);
        sprite.set_z_order(1);
        _card_sprites.push_back(bn::move(sprite));
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
