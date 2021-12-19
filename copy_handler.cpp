#include "copy_handler.hpp"
#include "string_helpers.hpp"
#include <vec/vec.hpp>
#include <iostream>
#include <toolkit/clipboard.hpp>

copy_handler global_copy_handler;

bool copy_handler::char_is_within_select_box(vec2f pos, vec2f cdim)
{
    if(copy_suppressed)
        return false;

    if(!held && !trigger_copy())
        return false;

    if((copy_end - copy_start).squared_length() < (MIN_SELECT_DISTANCE * MIN_SELECT_DISTANCE))
        return false;

    vec2f p1 = pos;
    vec2f p2 = pos + cdim;

    auto [tl, br] = points_to_rect(copy_start, copy_end);

    return rect_intersect(p1, p2, tl, br);
}

bool copy_handler::char_dragged_over(vec2f pos, vec2f cdim)
{
    if(copy_suppressed)
        return false;

    if(!held)
        return false;

    if((copy_end - copy_start).squared_length() < (MIN_SELECT_DISTANCE * MIN_SELECT_DISTANCE))
        return false;

    vec2f p1 = pos;
    vec2f p2 = pos + cdim;

    auto [tl, br] = points_to_rect(copy_start, copy_end);

    return rect_intersect(p1, p2, tl, br);
}

void copy_handler::on_lclick(vec2f pos)
{
    copy_start = pos;
    finished = false;
}

void copy_handler::on_lclick_release(vec2f pos)
{
    copy_end = pos;

    held = false;

    if(!copy_suppressed)
        finished = true;

    copy_suppressed = false;
}

void copy_handler::on_hold_lclick(vec2f pos)
{
    held = true;
    copy_end = pos;
}

void copy_handler::on_no_lclick()
{
    copy_suppressed = false;
}

bool copy_handler::trigger_copy()
{
    if(!finished)
        return false;

    bool trigger = false;

    if((copy_end - copy_start).squared_length() >= (MIN_SELECT_DISTANCE * MIN_SELECT_DISTANCE))
    {
        trigger = true;
    }

    //finished = false;
    //held = false;

    return trigger;
}

void copy_handler::set_clipboard(const std::string& str)
{
    clipboard::set(str);

    finished = false;
    held = false;
}

void copy_handler::tick(vec2f pos)
{

}

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
