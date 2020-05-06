#include "serialisables.hpp"

#include <networking/serialisable.hpp>
#include "imgui_ui_components.hpp"

DEFINE_SERIALISE_FUNCTION(chat_thread)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(raw_history);
    DO_FSERIALISE(command);

    if(ctx.serialisation && !ctx.encode)
    {
        me->history.clear();

        for(auto& i : me->raw_history)
        {
            me->history.push_back(string_to_interop_no_autos(i, false));
        }

        limit_size(me->raw_history, MAX_TEXT_HISTORY);
        limit_size(me->history, MAX_TEXT_HISTORY);
    }
}

DEFINE_SERIALISE_FUNCTION(interop_char)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(c);
    DO_FSERIALISE(col);
    DO_FSERIALISE(is_cursor);
    DO_FSERIALISE(coloured);
}

DEFINE_SERIALISE_FUNCTION(terminal_imgui)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(raw_history);
    DO_FSERIALISE(command);
    DO_FSERIALISE(new_terminal);
    DO_FSERIALISE(friendly_id);

    if(ctx.serialisation && !ctx.encode)
    {
        me->history.clear();

        for(auto& i : me->raw_history)
        {
            me->history.push_back(string_to_interop_no_autos(i, false));
        }

        limit_size(me->raw_history, MAX_TEXT_HISTORY);
        limit_size(me->history, MAX_TEXT_HISTORY);
    }
}

DEFINE_SERIALISE_FUNCTION(chat_window)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(chat_threads);
    DO_FSERIALISE(render_start);
    DO_FSERIALISE(side_buttons);
    DO_FSERIALISE(show_chat_in_main_window);
}

DEFINE_SERIALISE_FUNCTION(terminal_manager)
{
    SERIALISE_SETUP();

    if(ctx.serialisation && !ctx.encode)
    {
        me->auto_handle.found_unprocessed_autocompletes.clear();
        me->auto_handle.found_args.clear();
        me->auto_handle.is_valid.clear();
    }

    DO_FSERIALISE(sub_terminals);
    DO_FSERIALISE(main_terminal);
    DO_FSERIALISE(gid);
    DO_FSERIALISE(auto_handle);
}
