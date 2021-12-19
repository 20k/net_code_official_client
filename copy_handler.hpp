#ifndef COPY_HANDLER_HPP_INCLUDED
#define COPY_HANDLER_HPP_INCLUDED

#include <vec/vec.hpp>

struct formatted_char;

#define MIN_SELECT_DISTANCE 2.f

struct copy_handler
{
    std::string copied;
    float last_copy_y = 0;

    vec2f copy_start;
    vec2f copy_end;
    bool finished = true;
    bool held = false;
    bool copy_suppressed = false;

    void on_lclick(vec2f pos);
    void on_lclick_release(vec2f pos);

    void on_hold_lclick(vec2f pos);
    void on_no_lclick();

    bool trigger_copy();
    void set_clipboard(const std::string& str);

    void tick(vec2f pos);

    bool char_is_within_select_box(vec2f pos, vec2f cdim);
    bool char_dragged_over(vec2f pos, vec2f cdim);
};

extern copy_handler global_copy_handler;

inline
copy_handler* get_global_copy_handler()
{
    return &global_copy_handler;
}

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
