#include "format_cache.hpp"
#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>

void format_cache::ensure_built(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns)
{
    if(valid_cache)
        return;

    cache.clear();
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

        if(current_line + found_lines >= min_bound && current_line < max_bound)
        {
            auto current_interop = format_characters(all_interop[i], current, start, wrap_dim, found_lines, last_lines);

            cache.push_back(current_interop);
        }

        current.y() += found_lines * char_inf::cheight;

        last_lines = found_lines;
        current_line += found_lines;
    }

    internally_format(cache, {start.x(), start.y() + ImGui::GetWindowHeight()}, 0*scroll_hack.scrolled * char_inf::cheight, terminating_y);

    int height = ImGui::GetWindowHeight();

    for(auto it = cache.begin(); it != cache.end(); it++)
    {
        it->erase(std::remove_if(it->begin(), it->end(),
                                 [&](const formatted_char& chr)
                                 {
                                     return chr.render_pos.y() < start.y() + -char_inf::cheight || chr.render_pos.y() >= start.y() + height;
                                 }), it->end());
    }

    valid_cache = true;
}
