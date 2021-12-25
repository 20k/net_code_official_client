#ifndef CONTEXT_HPP_INCLUDED
#define CONTEXT_HPP_INCLUDED

#include <string>
#include "font_cfg.hpp"

struct context
{
    std::string user;
    std::string root_user;

    bool show_chat_in_main_window = true;
    bool should_shutdown = false;

    font_selector font_select;
};

#endif // CONTEXT_HPP_INCLUDED
