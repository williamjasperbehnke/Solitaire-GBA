#include "solitaire/app/hint_overlay.h"

namespace solitaire
{

    void hint_overlay::update()
    {
        if(_frames > 0)
        {
            --_frames;
        }
    }

    void hint_overlay::clear()
    {
        _frames = 0;
    }

    void hint_overlay::show(const klondike_hint_service::hint_result& hint, int duration_frames)
    {
        _hint = hint;
        _frames = duration_frames;
    }

    bool hint_overlay::visible() const
    {
        return _frames > 0;
    }

    const bn::string<48>* hint_overlay::text() const
    {
        return visible() ? &_hint.text : nullptr;
    }

    game_renderer::hint_highlight hint_overlay::highlight() const
    {
        game_renderer::hint_highlight output;
        if(visible())
        {
            output.has_move = _hint.has_move;
            output.from = _hint.move_from;
            output.to = _hint.move_to;
        }
        return output;
    }

    void hint_overlay::apply_selection(table_selection& selection) const
    {
        if(visible() && _hint.has_selection)
        {
            selection.set_selected_pile(_hint.selection, _hint.tableau_pick_depth_from_top);
        }
    }

}
