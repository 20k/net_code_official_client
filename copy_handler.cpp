#include "copy_handler.hpp"
#include "string_helpers.hpp"
#include <vec/vec.hpp>
#include <iostream>
#include <toolkit/clipboard.hpp>

void copy_handler2::on_lclick(vec2f pos)
{
    start = pos;
    triggered = false;
}

void copy_handler2::on_hold_lclick(vec2f pos)
{
    held = true;
    finish = pos;
}

void copy_handler2::on_lclick_release(vec2f pos)
{
    finish = pos;
    held = false;

    if(!cancelled)
        triggered = true;

    cancelled = false;
}

void copy_handler2::on_no_lclick()
{
    held = false;
    cancelled = false;
}

void copy_handler2::set_clipboard(const std::string& str)
{
    clipboard::set(str);
}

void copy_handler2::reset_trigger()
{
    triggered = false;
}

bool copy_handler2::is_dragging()
{
    return held;
}

bool copy_handler2::should_initiate_copy()
{
    return triggered;
}

bool copy_handler2::char_within_region(vec2f pos, vec2f cdim)
{
    if((start - finish).squared_length() < (MIN_SELECT_DISTANCE * MIN_SELECT_DISTANCE))
        return false;

    vec2f p1 = pos;
    vec2f p2 = pos + cdim;

    auto [tl, br] = points_to_rect(start, finish);

    return rect_intersect(p1, p2, tl, br);
}

copy_handler2& get_global_copy_handler2()
{
    static copy_handler2 cp;

    return cp;
}
