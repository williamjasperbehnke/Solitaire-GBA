#ifndef SOLITAIRE_HINT_OVERLAY_H
#define SOLITAIRE_HINT_OVERLAY_H

#include "solitaire/game/klondike_hint_service.h"
#include "solitaire/input/table_selection.h"
#include "solitaire/render/game_renderer.h"

namespace solitaire
{

    class hint_overlay
    {
    public:
        void update();
        void clear();
        void show(const klondike_hint_service::hint_result& hint, int duration_frames);
        [[nodiscard]] bool visible() const;

        [[nodiscard]] const bn::string<48>* text() const;
        [[nodiscard]] game_renderer::hint_highlight highlight() const;

        void apply_selection(table_selection& selection) const;

    private:
        int _frames = 0;
        klondike_hint_service::hint_result _hint;
    };

}

#endif
