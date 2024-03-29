#ifndef SERIALISABLES_HPP_INCLUDED
#define SERIALISABLES_HPP_INCLUDED

#include <networking/serialisable_fwd.hpp>

DECLARE_SERIALISE_FUNCTION(render_string);
DECLARE_SERIALISE_FUNCTION(paragraph_string);
DECLARE_SERIALISE_FUNCTION(text_manager);
DECLARE_SERIALISE_FUNCTION(terminal2);
DECLARE_SERIALISE_FUNCTION(main_terminal2);
DECLARE_SERIALISE_FUNCTION(child_terminal);
DECLARE_SERIALISE_FUNCTION(terminal_manager2);
DECLARE_SERIALISE_FUNCTION(chat_thread2);
DECLARE_SERIALISE_FUNCTION(chat_manager);

#endif // SERIALISABLES_HPP_INCLUDED
