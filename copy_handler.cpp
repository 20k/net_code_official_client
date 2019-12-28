#include "copy_handler.hpp"
#include "string_helpers.hpp"
#include <vec/vec.hpp>
#include <iostream>
#include <toolkit/clipboard.hpp>

copy_handler global_copy_handler;

bool copy_handler::char_is_within_select_box(vec2f pos, vec2f cdim)
{
    if(!held && !trigger_copy())
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

    if(finished)
        return;

    finished = true;
}

void copy_handler::on_hold_lclick(vec2f pos)
{
    held = true;
    copy_end = pos;
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
