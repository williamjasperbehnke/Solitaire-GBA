#include "solitaire/render/table_animation_renderer.h"

#include "bn_sprite_palette_items_deck_shared_palette.h"

#include "solitaire/core/animation_timing.h"
#include "solitaire/core/card_sprite_registry.h"
#include "solitaire/core/table_layout.h"
#include "solitaire/render/render_constants.h"

namespace solitaire
{
    namespace
    {
        constexpr int waste_preview_count = render_constants::waste_preview_count;
        constexpr int waste_preview_step = render_constants::waste_preview_step;
        constexpr int tableau_face_down_step = render_constants::tableau_face_down_step;
        constexpr int deal_animation_cards = animation_timing::deal_cards;
        constexpr int deal_animation_frames_per_card = animation_timing::deal_frames_per_card;
        constexpr int cancel_stagger_frames = animation_timing::cancel_stagger_frames;
        constexpr int cancel_travel_frames = animation_timing::cancel_travel_frames;

        struct anim_card
        {
            bool is_face_up = false;
            card value = {};
            int x = 0;
            int y = 0;
        };

        [[nodiscard]] constexpr int ease_out_quad_256(int t256)
        {
            if(t256 <= 0)
            {
                return 0;
            }
            if(t256 >= 256)
            {
                return 256;
            }

            return ((512 * t256) - (t256 * t256)) >> 8;
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

        void draw_face_card(const card& value, int x, int y, bn::vector<bn::sprite_ptr, 128>& out_card_sprites)
        {
            bn::sprite_ptr sprite = card_face_item(value).create_sprite(x, y);
            sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
            sprite.set_bg_priority(1);
            out_card_sprites.push_back(bn::move(sprite));
        }

        void draw_back_card(int x, int y, bn::vector<bn::sprite_ptr, 128>& out_card_sprites)
        {
            bn::sprite_ptr sprite = card_back_item().create_sprite(x, y);
            sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
            sprite.set_bg_priority(1);
            out_card_sprites.push_back(bn::move(sprite));
        }

        void append_top_row_cards(const klondike_game& game, bn::vector<anim_card, 80>& out_cards)
        {
            if(game.stock_size() > 0)
            {
                out_cards.push_back({ false, {}, table_layout::stock_x, table_layout::top_row_y });
            }

            card top_card;
            for(int waste_offset = waste_preview_count - 1; waste_offset >= 0; --waste_offset)
            {
                if(game.waste_card_from_top(waste_offset, top_card))
                {
                    const int x = table_layout::waste_x + (((waste_preview_count - 1) - waste_offset) * waste_preview_step);
                    out_cards.push_back({ true, top_card, x, table_layout::top_row_y });
                }
            }

            for(int foundation_index = 0; foundation_index < 4; ++foundation_index)
            {
                if(game.foundation_top(foundation_index, top_card))
                {
                    const int x = table_layout::foundation_base_x + (foundation_index * table_layout::pile_x_step);
                    out_cards.push_back({ true, top_card, x, table_layout::top_row_y });
                }
            }
        }

        void append_tableau_cards(const klondike_game& game, bn::vector<anim_card, 80>& out_cards)
        {
            card value;
            for(int tableau_index = 0; tableau_index < 7; ++tableau_index)
            {
                const int x = table_layout::tableau_base_x + (tableau_index * table_layout::pile_x_step);
                const int face_down_count = game.tableau_face_down_size(tableau_index);
                const int face_up_count = game.tableau_face_up_size(tableau_index);
                const int face_up_step = render_constants::tableau_face_up_step_for_count(face_up_count);

                for(int down_index = 0; down_index < face_down_count; ++down_index)
                {
                    const int y = table_layout::tableau_base_y + (down_index * tableau_face_down_step);
                    out_cards.push_back({ false, {}, x, y });
                }

                for(int up_index = 0; up_index < face_up_count; ++up_index)
                {
                    if(game.tableau_face_up_card(tableau_index, up_index, value))
                    {
                        const int y = table_layout::tableau_base_y + (face_down_count * tableau_face_down_step) +
                                      (up_index * face_up_step);
                        out_cards.push_back({ true, value, x, y });
                    }
                }
            }
        }
    }

    void table_animation_renderer::render_deal(const klondike_game& game, int deal_animation_frame,
                                               bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const
    {
        int dealt_cards = deal_animation_frame / deal_animation_frames_per_card;
        if(dealt_cards > deal_animation_cards)
        {
            dealt_cards = deal_animation_cards;
        }

        const int moving_step = dealt_cards;
        const int step_frame = deal_animation_frame % deal_animation_frames_per_card;
        const int step_t256 = (step_frame * 256) / deal_animation_frames_per_card;
        const int eased_t256 = ease_out_quad_256(step_t256);

        if(dealt_cards < deal_animation_cards)
        {
            draw_back_card(table_layout::stock_x, table_layout::top_row_y, out_card_sprites);
        }

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
                    draw_face_card(face_up_card, x, y, out_card_sprites);
                }
            }
            else
            {
                draw_back_card(x, y, out_card_sprites);
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
            const int arc_height = 4;

            const int x = source_x + ((target_x - source_x) * eased_t256) / 256;
            int y = source_y + ((target_y - source_y) * eased_t256) / 256;
            const int arch_t = step_t256 <= 128 ? step_t256 : (256 - step_t256);
            y -= (arch_t * arc_height) / 128;
            draw_back_card(x, y, out_card_sprites);
        }
    }

    void table_animation_renderer::render_cancel(const klondike_game& game, int cancel_animation_frame,
                                                 bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const
    {
        bn::vector<anim_card, 80> cards;
        append_top_row_cards(game, cards);
        append_tableau_cards(game, cards);
        if(cards.empty())
        {
            return;
        }

        draw_back_card(table_layout::stock_x, table_layout::top_row_y, out_card_sprites);

        for(int index = 0; index < cards.size(); ++index)
        {
            const int order = cards.size() - 1 - index;
            const int start_frame = order * cancel_stagger_frames;
            if(cancel_animation_frame >= start_frame)
            {
                continue;
            }

            const anim_card& card_data = cards[index];
            if(card_data.is_face_up)
            {
                draw_face_card(card_data.value, card_data.x, card_data.y, out_card_sprites);
            }
            else
            {
                draw_back_card(card_data.x, card_data.y, out_card_sprites);
            }
        }

        for(int index = 0; index < cards.size(); ++index)
        {
            const int order = cards.size() - 1 - index;
            const int start_frame = order * cancel_stagger_frames;
            const int local_frame = cancel_animation_frame - start_frame;
            if(local_frame < 0 || local_frame >= cancel_travel_frames)
            {
                continue;
            }

            int progress = (local_frame * 256) / cancel_travel_frames;
            if(progress > 256)
            {
                progress = 256;
            }

            const int eased = ease_out_quad_256(progress);
            const anim_card& moving_card = cards[index];
            const int x = moving_card.x + ((table_layout::stock_x - moving_card.x) * eased) / 256;
            const int y = moving_card.y + ((table_layout::top_row_y - moving_card.y) * eased) / 256;
            if(moving_card.is_face_up)
            {
                draw_face_card(moving_card.value, x, y, out_card_sprites);
            }
            else
            {
                draw_back_card(x, y, out_card_sprites);
            }
        }
    }

    void table_animation_renderer::render_victory(const klondike_game& game, int victory_animation_frame,
                                                  bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const
    {
        bn::vector<anim_card, 80> cards;
        append_top_row_cards(game, cards);
        append_tableau_cards(game, cards);

        for(int index = 0; index < cards.size(); ++index)
        {
            const anim_card& card_data = cards[index];
            const int x = ((index * 29 + victory_animation_frame * (3 + (index % 3))) % 240) - 120;
            const int y = ((index * 47 + victory_animation_frame * (2 + (index % 4))) % 160) - 80;
            if(card_data.is_face_up)
            {
                draw_face_card(card_data.value, x, y, out_card_sprites);
            }
            else
            {
                draw_back_card(x, y, out_card_sprites);
            }
        }
    }

    void table_animation_renderer::render_to_foundation(
            const card& value, int source_x, int source_y, int foundation_index, int animation_frame, int total_frames,
            bn::vector<bn::sprite_ptr, 128>& out_card_sprites) const
    {
        if(total_frames <= 0)
        {
            return;
        }

        int t256 = (animation_frame * 256) / total_frames;
        if(t256 < 0)
        {
            t256 = 0;
        }
        else if(t256 > 256)
        {
            t256 = 256;
        }

        const int eased_t256 = ease_out_quad_256(t256);
        const int target_x = table_layout::foundation_base_x + (foundation_index * table_layout::pile_x_step);
        const int target_y = table_layout::top_row_y;

        const int x = source_x + ((target_x - source_x) * eased_t256) / 256;
        int y = source_y + ((target_y - source_y) * eased_t256) / 256;
        const int arc_t = t256 <= 128 ? t256 : (256 - t256);
        y -= (arc_t * 3) / 128;

        bn::sprite_ptr sprite = card_face_item(value).create_sprite(x, y);
        sprite.set_palette(bn::sprite_palette_items::deck_shared_palette);
        sprite.set_bg_priority(0);
        sprite.set_z_order(-2);
        out_card_sprites.push_back(bn::move(sprite));
    }
}
