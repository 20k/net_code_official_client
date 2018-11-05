#include "format_cache.hpp"
#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>

void format_cache::ensure_built(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns)
{
    if(valid_cache)
        return;

    cached_start = start;
    cached_dim = wrap_dim;

    full_cache.clear();
    render_cache.clear();
    total_lines = 0;

    int last_lines = 0;

    int current_line = 0;

    for(auto& i : all_interop)
    {
        int found_line = 0;
        int empty_last = 0;

        get_height(i, current, start, wrap_dim, found_line, empty_last);

        total_lines += found_line;
    }

    y_internal_pos_to_index.clear();

    float inverse_scroll_start = total_lines + 0 - scroll_hack.scrolled;

    //float terminating_line = inverse_scroll_start;
    float terminating_y = inverse_scroll_start * char_inf::cheight + start.y();

    for(int i=0; i < (int)all_interop.size(); i++)
    {
        int found_lines = 0;

        get_height(all_interop[i], current, start, wrap_dim, found_lines, last_lines);

        int min_bound = inverse_scroll_start - vertical_columns;
        int max_bound = inverse_scroll_start;

        //if(current_line + found_lines >= min_bound && current_line < max_bound)
        {
            auto current_interop = format_characters(all_interop[i], current, start, wrap_dim, found_lines, last_lines);

            full_cache.push_back(current_interop);
        }

        auto f_it = y_internal_pos_to_index.find((int)current.y());

        if(f_it == y_internal_pos_to_index.end())
        {
            y_internal_pos_to_index[(int)current.y()] = i;
        }

        current.y() += found_lines * char_inf::cheight;

        last_lines = found_lines;
        current_line += found_lines;
    }

    y_internal_pos_to_index[(int)current.y()] = (int)all_interop.size()-1;

    auto temp = y_internal_pos_to_index;

    for(auto it = y_internal_pos_to_index.begin(); it != y_internal_pos_to_index.end(); it++)
    {
        auto bit = it;
        it++;

        if(it == y_internal_pos_to_index.end())
            break;

        int line_1 = bit->first;
        int idx_1 = bit->second;

        int line_2 = it->first;

        for(int i=line_1; i < line_2; i++)
        {
            temp[i] = idx_1;
        }
    }

    y_internal_pos_to_index = temp;

    internally_format(full_cache, {start.x(), start.y() + ImGui::GetWindowHeight()}, 0*scroll_hack.scrolled * char_inf::cheight, terminating_y);

    cached_y_end = terminating_y;

    int height = ImGui::GetWindowHeight();

    //render_cache = full_cache;

    /*for(auto it = render_cache.begin(); it != render_cache.end(); it++)
    {
        it->erase(std::remove_if(it->begin(), it->end(),
                                 [&](const formatted_char& chr)
                                 {
                                     return chr.render_pos.y() < start.y() + -char_inf::cheight || chr.render_pos.y() >= start.y() + height;
                                 }), it->end());
    }*/

    cached_line_offset = 0;

    valid_cache = true;
}

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

    vec2f internal_position = {0, cached_y_end - cached_start.y() + char_inf::cheight * 1.5f - ImGui::GetWindowHeight()};

    vec2f render_end = cached_start;
    render_end.y() += height;

    //std::cout << "yint " << y_internal_pos_to_index.size() << std::endl;

    /*for(auto& i : y_internal_pos_to_index)
    {
        std::cout << "i f " << i.first << " i s " << i.second << std::endl;
    }*/

    for(int i=internal_position.y(); i < internal_position.y() + height; i++)
    {
        auto it = y_internal_pos_to_index.find(i);

        if(it == y_internal_pos_to_index.end())
            continue;

        auto bit = it;
        it++;

        int start_1 = bit->second;

        ///temp
        if(it == y_internal_pos_to_index.end())
        {
            ret.push_back(full_cache[start_1]);

            break;
        }

        int start_2 = it->second;

        //printf("s %i %i\n", start_1, start_2);

        for(int idx = start_1; idx < start_2; idx++)
        {
            assert(idx >= 0 && idx < full_cache.size());

            ret.push_back(full_cache[idx]);
        }
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
