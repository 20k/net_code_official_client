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

    if(ctx.serialisation && !ctx.encode)
    {
        me->auto_handle.found_unprocessed_autocompletes.clear();
        me->auto_handle.found_args.clear();
        me->auto_handle.is_valid.clear();
    }

    DO_FSERIALISE(raw_history);
    DO_FSERIALISE(chat_threads);
    DO_FSERIALISE(command);
    DO_FSERIALISE(auto_handle);

    if(ctx.serialisation && !ctx.encode)
    {
        me->history.clear();

        for(auto& i : me->raw_history)
        {
            me->history.push_back(string_to_interop_no_autos(i, false));
        }
    }
}
