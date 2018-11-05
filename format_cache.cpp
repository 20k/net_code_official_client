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

        current.y() += found_lines * char_inf::cheight;

        last_lines = found_lines;
        current_line += found_lines;
    }

    internally_format(full_cache, {start.x(), start.y() + ImGui::GetWindowHeight()}, 0*scroll_hack.scrolled * char_inf::cheight, terminating_y);

    cached_y_end = terminating_y;

    cached_line_offset = 0;

    valid_cache = true;
}

/*std::vector<formatted_char> format_cache::request(int render_y_start, int render_y_end)
{
    std::vector<formatted_char> ret;

    int height = ImGui::GetWindowHeight();

    vec2f render_position = cached_start;
    render_position.y() += cached_line_offset * char_inf::cheight;

    vec2f internal_position = {0, cached_y_end - cached_start.y() + char_inf::cheight * 1.5f - ImGui::GetWindowHeight() - cached_line_offset * char_inf::cheight};

    set::set<int> fetch_list;
}*/

std::vector<std::vector<formatted_char>> format_cache::get_render_cache()
{
    std::vector<std::vector<formatted_char>> ret;

    int height = ImGui::GetWindowHeight();

    vec2f render_position = cached_start;
    render_position.y() -= cached_line_offset * char_inf::cheight;

    for(auto& i : full_cache)
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
                ret.push_back(i);
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
