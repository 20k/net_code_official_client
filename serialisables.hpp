#ifndef SERIALISABLES_HPP_INCLUDED
#define SERIALISABLES_HPP_INCLUDED

#include <networking/serialisable_fwd.hpp>

struct terminal_imgui;

DECLARE_SERIALISE_FUNCTION(terminal_imgui);
DECLARE_SERIALISE_FUNCTION(chat_window);

#endif // SERIALISABLES_HPP_INCLUDED
