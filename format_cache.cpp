#include "format_cache.hpp"
#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>
#include <iostream>
#include "copy_handler.hpp"
#include "string_helpers.hpp"

void format_cache_2::ensure_built(vec2f window_dimensions, const std::vector<interop_vec_t>& all_chars)
{
    vec2f cdim = xy_to_vec(ImGui::CalcTextSize("A"));

    vec2f start = {char_inf::cwbuf, 0};

    if(valid_cache && !valid_last_line)
    {
        valid_last_line = true;

        if(all_chars.size() == 0 || line_cache.size() == 0 || height_cache.size() == 0)
        {
            invalidate();
            return ensure_built(window_dimensions, all_chars);
        }

        line_cache.pop_back();
        height_cache.pop_back();

        int last_line = 0;

        for(auto& i : height_cache)
            last_line += i;

        int found_line = 0;

        vec2f current = start + (vec2f){0, cdim.y() * last_line};

        get_height(all_chars.back(), current, start, window_dimensions, found_line, cdim);

        height_cache.push_back(found_line);
        line_cache.push_back(format_characters(all_chars.back(), current, start, window_dimensions, found_line, last_line, cdim));
        return;
    }

    if(valid_cache)
        return;

    line_cache.clear();
    height_cache.clear();
    int last_line = 0;

    vec2f current = {char_inf::cwbuf, 0};

    for(int i=0; i < (int)all_chars.size(); i++)
    {
        int found_line = 0;

        get_height(all_chars[i], current, start, window_dimensions, found_line, cdim);

        height_cache.push_back(found_line);

        auto cur_formatted = format_characters(all_chars[i], current, start, window_dimensions, found_line, last_line, cdim);
        last_line = found_line;
        current.y() += found_line * cdim.y();

        line_cache.push_back(cur_formatted);
    }

    valid_cache = true;
    valid_last_line = true;
}

void render_raw(const std::string& str, vec3f col, vec2f render_pos)
{
    if(ImGui::IsStyleLinearColor())
        col = srgb_to_lin(col/255.f) * 255.f;

    ImDrawList* imlist = ImGui::GetWindowDrawList();
    imlist->AddText(ImVec2(render_pos.x(), render_pos.y()), IM_COL32((int)col.x(), (int)col.y(), (int)col.z(), 255), str.c_str());
}

void render_indices(vec2f screen_pos, int& idx_1, int idx_2, const std::vector<formatted_char>& text, bool check_copy, float screen_cull_y_bottom, float screen_cull_y_top)
{
    if(idx_1 == idx_2)
        return;

    vec2f final_pos = screen_pos + text[idx_1].internal_pos;
    vec2f start_check_pos = screen_pos + text[idx_2 - 1].internal_pos;

    vec2f cdim = {char_inf::cached_imgui_width, char_inf::cached_imgui_height};

    if(final_pos.y() - cdim.y() >= screen_cull_y_bottom)
        return;

    if(start_check_pos.y() + cdim.y() <= screen_cull_y_top)
        return;

    std::string str;
    str.reserve(idx_2 - idx_1);
    vec3f col;

    for(int i=idx_1; i < idx_2; i++)
    {
        str += text[i].ioc.c;
    }

    col = text[idx_1].ioc.col;

    copy_handler* handle = get_global_copy_handler();

    if(ImGui::IsWindowFocused() && handle->char_is_within_select_box(screen_pos + text[idx_1].internal_pos, cdim) && text[idx_1].copyable)
    {
        if(check_copy)
        {
            if(handle->copied.size() != 0 && handle->last_copy_y != (screen_pos.y() + text[idx_1].internal_pos.y()))
                handle->copied += "\n" + str;
            else
                handle->copied += str;

            handle->last_copy_y = (screen_pos.y() + text[idx_1].internal_pos.y());
        }

        col = {80, 80, 255};

        for(auto& i : str)
        {
            if(i == ' ')
                i = '-';
        }
    }

    render_raw(str, col, final_pos);

    idx_1 = idx_2;
}

void render_formatted(vec2f screen_pos, const std::vector<formatted_char>& text, bool check_copy, float screen_cull_y_bottom, float screen_cull_y_top, vec2f cdim)
{
    copy_handler* handle = get_global_copy_handler();

    int idx = 0;
    int lidx = 0;

    for(; idx < (int)text.size() - 1; idx++)
    {
        const formatted_char& cur = text[idx];
        const formatted_char& next = text[idx + 1];

        if(cur.ioc.is_cursor)
        {
            ///render previous string
            render_indices(screen_pos, lidx, idx, text, check_copy, screen_cull_y_bottom, screen_cull_y_top);
            ///render cursor
            render_indices(screen_pos, lidx, idx + 1, text, check_copy, screen_cull_y_bottom, screen_cull_y_top);
            continue;
        }

        vec2f p1 = cur.internal_pos + screen_pos;
        vec2f p2 = next.internal_pos + screen_pos;

        if(cur.ioc.col != next.ioc.col ||
           cur.internal_pos.y() != next.internal_pos.y() ||
           handle->char_is_within_select_box(p1, cdim) != handle->char_is_within_select_box(p2, cdim))
        {
            render_indices(screen_pos, lidx, idx+1, text, check_copy, screen_cull_y_bottom, screen_cull_y_top);
            continue;
        }
    }

    render_indices(screen_pos, lidx, (int)text.size(), text, check_copy, screen_cull_y_bottom, screen_cull_y_top);
}

void format_cache_2::render_imgui(vec2f position, vec2f dim, float scroll_lines)
{
    copy_handler* handle = get_global_copy_handler();

    int total_lines = 0;

    for(auto& i : height_cache)
        total_lines += i;

    bool do_copy = false;

    if(ImGui::IsWindowFocused())
    {
        if(handle->trigger_copy())
        {
            do_copy = true;
            handle->copied = "";
        }
    }

    vec2f cdim = xy_to_vec(ImGui::CalcTextSize("A"));

    float vertical_fudge_for_floating_terminal = cdim.y() * 2;

    if(line_cache.size() > 0)
    {
        vertical_fudge_for_floating_terminal += height_cache.back() * cdim.y();
    }

    vec2f mdim = {dim.x(), dim.y() - vertical_fudge_for_floating_terminal};

    bool has_last_content_size = false;
    vec2f content_start = {0,0};
    vec2f content_end = {0,0};

    float vertical_offset = scroll_lines * cdim.y() - total_lines * cdim.y() + dim.y() - cdim.y()*1.5;

    float screen_cull_y_bottom = position.y() + mdim.y();
    float screen_cull_y_top = position.y();

    for(int i=0; i < (int)line_cache.size() - 1; i++)
    {
        if(line_cache[i].size() == 0)
            continue;

        vec2f display_first = line_cache[i].front().internal_pos + position;
        vec2f display_last = line_cache[i].back().internal_pos + position;

        display_first.y() += vertical_offset;
        display_last.y() += vertical_offset;

        if(!has_last_content_size)
        {
            content_start = display_first;
        }

        has_last_content_size = true;

        content_end = display_last;

        if(display_last.y() < position.y() - cdim.y())
            continue;

        if(display_first.y() > position.y() + mdim.y() + cdim.y())
            continue;

        ///render!
        render_formatted(position + (vec2f){0, vertical_offset}, line_cache[i], do_copy, screen_cull_y_bottom, screen_cull_y_top, cdim);
    }

    if(line_cache.size() > 0)
    {
        std::vector<formatted_char>& cformat = line_cache.back();

        if(cformat.size() > 0)
        {
            ///not correct
            content_end = cformat.back().internal_pos + position;

            vec2f display_pos = position + (vec2f){0, dim.y()} - (vec2f){0, cdim.y() * 0.5};

            display_pos.y() -= height_cache.back() * cdim.y();
            display_pos.y() += 4;

            vec2f relative_pos = (vec2f){display_pos.x(), display_pos.y() - cformat.front().internal_pos.y()};

            render_formatted(relative_pos, line_cache.back(), do_copy, position.y() + dim.y(), screen_cull_y_top, cdim);
        }
    }

    if(do_copy)
    {
        std::cout << "Copied " << handle->copied << std::endl;

        handle->set_clipboard(handle->copied);
    }

    last_content_size = (vec2f){dim.x(), (content_end - content_start).y()};
    valid_visual_cache = true;
}
