#ifndef CONTEXT_HPP_INCLUDED
#define CONTEXT_HPP_INCLUDED

#include <string>
#include "font_cfg.hpp"
#include "render_string.hpp"

struct generic_backend;

struct context
{
    std::string user;
    std::string root_user;

    bool show_chat_in_main_window = true;
    bool should_shutdown = false;

    font_selector font_select;
    generic_backend* backend = nullptr;
    terminal_manager2 terminals;
};

#endif // CONTEXT_HPP_INCLUDED
