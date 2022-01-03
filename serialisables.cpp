#include "serialisables.hpp"

#include <networking/serialisable.hpp>
#include "render_string.hpp"

DEFINE_SERIALISE_FUNCTION(render_string)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(colour);
    DO_FSERIALISE(start);
    DO_FSERIALISE(length);
}

DEFINE_SERIALISE_FUNCTION(paragraph_string)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(unformatted_char_width);
    DO_FSERIALISE(str);
    DO_FSERIALISE(basic_render_strings);
}

DEFINE_SERIALISE_FUNCTION(text_manager)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(unseen_text);
    DO_FSERIALISE(friendly_id);
    DO_FSERIALISE(paragraphs);
    DO_FSERIALISE(command);
}

DEFINE_SERIALISE_FUNCTION(terminal2)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(unseen_text);
    DO_FSERIALISE(friendly_id);
    DO_FSERIALISE(paragraphs);
    DO_FSERIALISE(tag);
    DO_FSERIALISE(command);
}

DEFINE_SERIALISE_FUNCTION(main_terminal2)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(unseen_text);
    DO_FSERIALISE(friendly_id);
    DO_FSERIALISE(paragraphs);
    DO_FSERIALISE(tag);
    DO_FSERIALISE(command);
}

DEFINE_SERIALISE_FUNCTION(child_terminal)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(unseen_text);
    DO_FSERIALISE(friendly_id);
    DO_FSERIALISE(paragraphs);
    DO_FSERIALISE(tag);
    DO_FSERIALISE(command);
}

DEFINE_SERIALISE_FUNCTION(terminal_manager2)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(primary);
    DO_FSERIALISE(secondary);
}

DEFINE_SERIALISE_FUNCTION(chat_thread2)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(unseen_text);
    DO_FSERIALISE(friendly_id);
    DO_FSERIALISE(paragraphs);
    DO_FSERIALISE(friendly_name);
    DO_FSERIALISE(command);
}

DEFINE_SERIALISE_FUNCTION(chat_manager)
{
    SERIALISE_SETUP();

    DO_FSERIALISE(chat_threads);
    DO_FSERIALISE(open_chat_channels);
}
