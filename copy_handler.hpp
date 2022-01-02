#ifndef COPY_HANDLER_HPP_INCLUDED
#define COPY_HANDLER_HPP_INCLUDED

#include <vec/vec.hpp>

struct formatted_char;

#define MIN_SELECT_DISTANCE 2.f

struct copy_handler2
{
    float last_copy_y = 0;

    bool triggered = false;
    bool held = false;
    bool cancelled = false;

    vec2f start;
    vec2f finish;

    void on_lclick(vec2f pos);
    void on_lclick_release(vec2f pos);

    void on_hold_lclick(vec2f pos);
    void on_no_lclick();

    void set_clipboard(const std::string& str);
    void reset_trigger();

    bool is_dragging();
    bool should_initiate_copy();

    bool char_within_region(vec2f pos, vec2f cdim);
};

copy_handler2& get_global_copy_handler2();

#endif // COPY_HANDLER_HPP_INCLUDED
