#include "solitaire/core/card_sprite_registry.h"

#include "bn_sprite_items_card_a_c.h"
#include "bn_sprite_items_card_a_d.h"
#include "bn_sprite_items_card_a_h.h"
#include "bn_sprite_items_card_a_s.h"
#include "bn_sprite_items_card_2_c.h"
#include "bn_sprite_items_card_2_d.h"
#include "bn_sprite_items_card_2_h.h"
#include "bn_sprite_items_card_2_s.h"
#include "bn_sprite_items_card_3_c.h"
#include "bn_sprite_items_card_3_d.h"
#include "bn_sprite_items_card_3_h.h"
#include "bn_sprite_items_card_3_s.h"
#include "bn_sprite_items_card_4_c.h"
#include "bn_sprite_items_card_4_d.h"
#include "bn_sprite_items_card_4_h.h"
#include "bn_sprite_items_card_4_s.h"
#include "bn_sprite_items_card_5_c.h"
#include "bn_sprite_items_card_5_d.h"
#include "bn_sprite_items_card_5_h.h"
#include "bn_sprite_items_card_5_s.h"
#include "bn_sprite_items_card_6_c.h"
#include "bn_sprite_items_card_6_d.h"
#include "bn_sprite_items_card_6_h.h"
#include "bn_sprite_items_card_6_s.h"
#include "bn_sprite_items_card_7_c.h"
#include "bn_sprite_items_card_7_d.h"
#include "bn_sprite_items_card_7_h.h"
#include "bn_sprite_items_card_7_s.h"
#include "bn_sprite_items_card_8_c.h"
#include "bn_sprite_items_card_8_d.h"
#include "bn_sprite_items_card_8_h.h"
#include "bn_sprite_items_card_8_s.h"
#include "bn_sprite_items_card_9_c.h"
#include "bn_sprite_items_card_9_d.h"
#include "bn_sprite_items_card_9_h.h"
#include "bn_sprite_items_card_9_s.h"
#include "bn_sprite_items_card_10_c.h"
#include "bn_sprite_items_card_10_d.h"
#include "bn_sprite_items_card_10_h.h"
#include "bn_sprite_items_card_10_s.h"
#include "bn_sprite_items_card_j_c.h"
#include "bn_sprite_items_card_j_d.h"
#include "bn_sprite_items_card_j_h.h"
#include "bn_sprite_items_card_j_s.h"
#include "bn_sprite_items_card_q_c.h"
#include "bn_sprite_items_card_q_d.h"
#include "bn_sprite_items_card_q_h.h"
#include "bn_sprite_items_card_q_s.h"
#include "bn_sprite_items_card_k_c.h"
#include "bn_sprite_items_card_k_d.h"
#include "bn_sprite_items_card_k_h.h"
#include "bn_sprite_items_card_k_s.h"
#include "bn_sprite_items_card_back.h"

namespace solitaire
{

    const bn::sprite_item& card_back_item()
    {
        return bn::sprite_items::card_back;
    }

    const bn::sprite_item& card_face_item(const card& value)
    {
        switch(value.card_suit)
        {
            case suit::clubs:
                switch(value.rank)
                {
                    case 1: return bn::sprite_items::card_a_c;
                    case 2: return bn::sprite_items::card_2_c;
                    case 3: return bn::sprite_items::card_3_c;
                    case 4: return bn::sprite_items::card_4_c;
                    case 5: return bn::sprite_items::card_5_c;
                    case 6: return bn::sprite_items::card_6_c;
                    case 7: return bn::sprite_items::card_7_c;
                    case 8: return bn::sprite_items::card_8_c;
                    case 9: return bn::sprite_items::card_9_c;
                    case 10: return bn::sprite_items::card_10_c;
                    case 11: return bn::sprite_items::card_j_c;
                    case 12: return bn::sprite_items::card_q_c;
                    case 13: return bn::sprite_items::card_k_c;
                    default: return bn::sprite_items::card_back;
                }
            case suit::diamonds:
                switch(value.rank)
                {
                    case 1: return bn::sprite_items::card_a_d;
                    case 2: return bn::sprite_items::card_2_d;
                    case 3: return bn::sprite_items::card_3_d;
                    case 4: return bn::sprite_items::card_4_d;
                    case 5: return bn::sprite_items::card_5_d;
                    case 6: return bn::sprite_items::card_6_d;
                    case 7: return bn::sprite_items::card_7_d;
                    case 8: return bn::sprite_items::card_8_d;
                    case 9: return bn::sprite_items::card_9_d;
                    case 10: return bn::sprite_items::card_10_d;
                    case 11: return bn::sprite_items::card_j_d;
                    case 12: return bn::sprite_items::card_q_d;
                    case 13: return bn::sprite_items::card_k_d;
                    default: return bn::sprite_items::card_back;
                }
            case suit::hearts:
                switch(value.rank)
                {
                    case 1: return bn::sprite_items::card_a_h;
                    case 2: return bn::sprite_items::card_2_h;
                    case 3: return bn::sprite_items::card_3_h;
                    case 4: return bn::sprite_items::card_4_h;
                    case 5: return bn::sprite_items::card_5_h;
                    case 6: return bn::sprite_items::card_6_h;
                    case 7: return bn::sprite_items::card_7_h;
                    case 8: return bn::sprite_items::card_8_h;
                    case 9: return bn::sprite_items::card_9_h;
                    case 10: return bn::sprite_items::card_10_h;
                    case 11: return bn::sprite_items::card_j_h;
                    case 12: return bn::sprite_items::card_q_h;
                    case 13: return bn::sprite_items::card_k_h;
                    default: return bn::sprite_items::card_back;
                }
            case suit::spades:
                switch(value.rank)
                {
                    case 1: return bn::sprite_items::card_a_s;
                    case 2: return bn::sprite_items::card_2_s;
                    case 3: return bn::sprite_items::card_3_s;
                    case 4: return bn::sprite_items::card_4_s;
                    case 5: return bn::sprite_items::card_5_s;
                    case 6: return bn::sprite_items::card_6_s;
                    case 7: return bn::sprite_items::card_7_s;
                    case 8: return bn::sprite_items::card_8_s;
                    case 9: return bn::sprite_items::card_9_s;
                    case 10: return bn::sprite_items::card_10_s;
                    case 11: return bn::sprite_items::card_j_s;
                    case 12: return bn::sprite_items::card_q_s;
                    case 13: return bn::sprite_items::card_k_s;
                    default: return bn::sprite_items::card_back;
                }
            default:
                return bn::sprite_items::card_back;
        }
    }

}
