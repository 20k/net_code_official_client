#include "format_cache.hpp"
#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>
#include <iostream>
#include "copy_handler.hpp"

void format_cache::ensure_last_line(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns)
{
    if(!last_line_valid && valid_cache)
    {
        if(interop_cache.size() == 0)
            return;

        //interop_cache = all_interop;

        if(all_interop.size() == 0)
            return;

        int found_line = 0;

        get_height(all_interop.back(), cached_start, cached_start, cached_dim, found_line);

        int old_height = height_map_cache[(int)interop_cache.size()-1];

        if(old_height != found_line)
            deferred_invalidate = true;

        total_lines -= old_height;
        total_lines += found_line;

        height_map_cache[(int)interop_cache.size()-1] = found_line;

        if(interop_cache.size() > 0)
            initialised_cache[(int)interop_cache.size()-1] = false;

        interop_cache.back() = all_interop.back();

        last_line_valid = true;
        return;
    }
}

void format_cache::ensure_built(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns)
{
    ensure_last_line(current, start, wrap_dim, all_interop, scroll_hack, vertical_columns);

    if(deferred_invalidate)
    {
        valid_cache = false;
        deferred_invalidate = false;
        return;
    }

    if(valid_cache)
        return;

    cached_start = start;
    cached_dim = wrap_dim;
    cached_window_size = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};

    total_lines = 0;

    height_map_cache.clear();

    interop_cache = all_interop;

    for(int i=0; i < (int)all_interop.size(); i++)
    {
        int found_line = 0;

        get_height(all_interop[i], current, start, wrap_dim, found_line);

        height_map_cache[i] = found_line;

        total_lines += found_line;
    }

    float inverse_scroll_start = total_lines + 0 - scroll_hack.scrolled;

    scrolled_cache = scroll_hack.scrolled;

    //float terminating_line = inverse_scroll_start;
    float terminating_y = inverse_scroll_start * char_inf::cheight + start.y();

    initialised_cache.clear();
    line_cache.clear();

    cached_y_end = terminating_y;

    cached_line_offset = 0;

    valid_cache = true;
    last_line_valid = true;
}

std::vector<std::vector<formatted_char>> format_cache::get_render_cache()
{
    std::vector<std::vector<formatted_char>> ret;

    float height = ImGui::GetWindowHeight();

    vec2f render_position = cached_start;
    render_position.y() -= cached_line_offset * char_inf::cheight;

    float inverse_scroll_start = total_lines - (scrolled_cache + cached_line_offset);

    std::vector<std::vector<formatted_char>> local_cache;

    int vertical_rows = ceil((float)height / char_inf::cheight);

    vec2f current = cached_start;

    int current_line = 0;

    int last_lines = 0;

    for(int i=0; i < (int)interop_cache.size(); i++)
    {
        int found_lines = height_map_cache[i];

        int min_bound = inverse_scroll_start - vertical_rows;
        int max_bound = inverse_scroll_start;

        if(current_line + found_lines >= min_bound && current_line < max_bound)
        {
            if(initialised_cache[i])
            {
                local_cache.push_back(line_cache[i]);
            }
            else
            {
                auto current_interop = format_characters(interop_cache[i], current, cached_start, cached_dim, found_lines, last_lines);

                initialised_cache[i] = true;
                line_cache[i] = current_interop;
                local_cache.push_back(current_interop);
            }
        }

        current.y() += found_lines * char_inf::cheight;

        last_lines = found_lines;
        current_line += found_lines;
    }

    //if(dirty)
    {
        //float terminating_y = inverse_scroll_start * char_inf::cheight + cached_start.y();

        internally_format(local_cache, {cached_start.x(), cached_start.y() + height}, 0.f, cached_y_end);
    }

    for(auto& i : local_cache)
    {
        if(i.size() == 0)
            continue;

        if(i.size() == 1)
        {
            formatted_char& chr = i[0];

            if(chr.render_pos.y() < render_position.y() - char_inf::cheight/2 || chr.render_pos.y() >= render_position.y() + height + char_inf::cheight/2)
                continue;

            ret.push_back(i);
            continue;
        }

        if(i.size() > 1)
        {
            formatted_char& first = i.front();
            formatted_char& last = i.back();

            if(last.render_pos.y() < render_position.y() - char_inf::cheight/2 || first.render_pos.y() >= render_position.y() + height + char_inf::cheight/2)
                continue;

            bool straddles = last.render_pos.y() > render_position.y() + height || first.render_pos.y() < render_position.y();

            if(straddles)
            {
                std::vector<formatted_char> accum;

                for(formatted_char& chr : i)
                {
                    float ypos = chr.render_pos.y();

                    if(ypos < render_position.y() - char_inf::cheight || ypos >= render_position.y() + height + char_inf::cheight)
                        continue;
                    else
                        accum.push_back(chr);
                }

                ret.push_back(accum);
            }
            else
            {
                ret.push_back(std::move(i));
            }

            for(auto& chr : ret.back())
            {
                chr.render_pos.y() += cached_line_offset * char_inf::cheight;
            }

            continue;
        }
    }

    return ret;
}

void format_cache_2::ensure_built(vec2f window_dimensions, const std::vector<interop_vec_t>& all_chars)
{
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

        vec2f current = start + (vec2f){0, char_inf::cheight * last_line};

        get_height(all_chars.back(), current, start, window_dimensions, found_line);

        height_cache.push_back(found_line);
        line_cache.push_back(format_characters(all_chars.back(), current, start, window_dimensions, found_line, last_line));
        return;
    }

    if(valid_cache)
        return;

    line_cache.clear();
    height_cache.clear();
    int last_line = 0;

    vec2f current = {char_inf::cwbuf,0};

    for(int i=0; i < all_chars.size(); i++)
    {
        int found_line = 0;

        get_height(all_chars[i], current, start, window_dimensions, found_line);

        height_cache.push_back(found_line);

        auto cur_formatted = format_characters(all_chars[i], current, start, window_dimensions, found_line, last_line);
        last_line = found_line;
        current.y() += found_line * char_inf::cheight;

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

void render_indices(vec2f screen_pos, int& idx_1, int idx_2, const std::vector<formatted_char>& text, bool check_copy)
{
    std::string str;
    vec3f col;

    if(idx_1 == idx_2)
        return;

    for(int i=idx_1; i < idx_2; i++)
    {
        str += text[i].ioc.c;
    }

    col = text[idx_1].ioc.col;

    copy_handler* handle = get_global_copy_handler();

    if(ImGui::IsWindowFocused() && handle->char_is_within_select_box(screen_pos + text[idx_1].internal_pos) && text[idx_1].copyable)
    {
        col = {80, 80, 255};

        for(auto& i : str)
        {
            if(i == ' ')
                i = '-';
        }

        if(check_copy)
        {
            if(get_global_copy_handler()->copied.size() != 0)
                get_global_copy_handler()->copied += "\n" + str;
            else
                get_global_copy_handler()->copied = str;
        }
    }

    render_raw(str, col, screen_pos + text[idx_1].internal_pos);

    idx_1 = idx_2;
}

void render_formatted(vec2f screen_pos, const std::vector<formatted_char>& text, bool check_copy)
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
            render_indices(screen_pos, lidx, idx, text, check_copy);
            ///render cursor
            render_indices(screen_pos, lidx, idx + 1, text, check_copy);
            continue;
        }

        vec2f p1 = cur.internal_pos + screen_pos;
        vec2f p2 = next.internal_pos + screen_pos;

        if(cur.ioc.col != next.ioc.col ||
           cur.internal_pos.y() != next.internal_pos.y() ||
           handle->char_is_within_select_box(p1) != handle->char_is_within_select_box(p2))
        {
            render_indices(screen_pos, lidx, idx+1, text, check_copy);
            continue;
        }
    }

    render_indices(screen_pos, lidx, (int)text.size(), text, check_copy);
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

    float vertical_offset = scroll_lines * char_inf::cheight - total_lines * char_inf::cheight + dim.y() - char_inf::cheight*1.5;

    for(int i=0; i < (int)line_cache.size(); i++)
    {
        if(line_cache[i].size() == 0)
            continue;

        vec2f display_first = line_cache[i].front().internal_pos + position;
        vec2f display_last = line_cache[i].back().internal_pos + position;

        display_first.y() += vertical_offset;
        display_last.y() += vertical_offset;

        if(display_last.y() < position.y() - char_inf::cheight)
            continue;

        if(display_first.y() > position.y() + dim.y() + char_inf::cheight)
            continue;

        ///render!
        render_formatted(position + (vec2f){0, vertical_offset}, line_cache[i], do_copy);
    }

    if(do_copy)
    {
        std::cout << "Copied " << handle->copied << std::endl;

        handle->set_clipboard(handle->copied);
    }
}

#if 0
std::vector<std::vector<formatted_char>> format_cache::get_render_cache()
{
    std::vector<std::vector<formatted_char>> ret;// = full_cache;

    int height = ImGui::GetWindowHeight();

    /*vec2f current_start = cached_start;

    current_start.y() += cached_line_offset * char_inf::cheight;

    float yoff = cached_line_offset * char_inf::cheight;

    for(auto it = ret.begin(); it != ret.end(); it++)
    {
        it->erase(std::remove_if(it->begin(), it->end(),
                                 [&](const formatted_char& chr)
                                 {
                                     return chr.render_pos.y() + yoff < cached_start.y() + -char_inf::cheight || chr.render_pos.y() + yoff >= cached_start.y() + height;
                                 }), it->end());
    }

    for(auto& i : ret)
    {
        for(formatted_char& j : i)
        {
            j.render_pos.y() += cached_line_offset * char_inf::cheight;
        }
    }*/

    vec2f render_position = cached_start;
    render_position.y() += cached_line_offset * char_inf::cheight;

    vec2f internal_position = {0, cached_y_end - cached_start.y() + char_inf::cheight * 1.5f - ImGui::GetWindowHeight() - cached_line_offset * char_inf::cheight};

    std::set<int> fetch_list;

    auto first_iterator = y_internal_pos_to_index.begin();
    auto last_iterator = y_internal_pos_to_index.end();

    bool has_first = false;

    int first_y_index = internal_position.y() - 1;

    int last_y_index = internal_position.y() + height + 1;

    int nidx = 0;

    std::cout <<" first y " << first_y_index << " last " << last_y_index << std::endl;

    for(int i=first_y_index; i < last_y_index; i++)
    {
        auto it = y_internal_pos_to_index.find(i);

        if(it == y_internal_pos_to_index.end())
            continue;

        nidx++;

        if(!has_first)
        {
            first_iterator = it;
            has_first = true;
        }

        auto bit = it;
        it++;

        int start_1 = bit->second;

        if(it == y_internal_pos_to_index.end())
        {
            fetch_list.insert(start_1);

            last_iterator = it;

            break;
        }

        last_iterator = it;

        int start_2 = it->second;

        //printf("s %i %i\n", start_1, start_2);

        ///need to also render one before, and render one after
        ///maybe construct a set which is a renderlist?
        for(int idx = start_1; idx <= start_2; idx++)
        {
            fetch_list.insert(idx);
        }
    }

    if(fetch_list.size() > 0)
    {
        int start = *fetch_list.begin();

        if(start > 0)
            fetch_list.insert(start-1);
    }

    std::cout << "flist size " << fetch_list.size() << " nidx " << nidx << std::endl;

    for(auto& i : fetch_list)
    {
        if(i < 0 || i >= (int)full_cache.size())
            continue;

        ret.push_back(full_cache[i]);
    }

    for(auto& i : ret)
    {
        for(formatted_char& j : i)
        {
            j.render_pos.y() += cached_line_offset * char_inf::cheight;
        }
    }

    return ret;
}
#endif // 0
