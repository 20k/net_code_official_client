#ifndef COPY_HANDLER_HPP_INCLUDED
#define COPY_HANDLER_HPP_INCLUDED

#include <vec/vec.hpp>

struct formatted_char;

#define MIN_SELECT_DISTANCE 2.f

struct copy_handler
{
    std::string copied;

    vec2f copy_start;
    vec2f copy_end;
    bool finished = true;
    bool held = false;

    void on_lclick(vec2f pos);
    void on_lclick_release(vec2f pos);

    void on_hold_lclick(vec2f pos);

    void process_formatted(std::vector<std::vector<formatted_char>>& chars);

    void tick(vec2f pos);

    bool char_is_within_select_box(vec2f pos);
};

extern copy_handler global_copy_handler;

inline
copy_handler* get_global_copy_handler()
{
    return &global_copy_handler;
}

#endif // COPY_HANDLER_HPP_INCLUDED
