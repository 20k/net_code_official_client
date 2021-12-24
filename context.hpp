#ifndef CONTEXT_HPP_INCLUDED
#define CONTEXT_HPP_INCLUDED

#include <string>

struct context
{
    std::string user;
    std::string root_user;

    bool show_chat_in_main_window = true;
};

#endif // CONTEXT_HPP_INCLUDED
