#include "render_string.hpp"
#include "colour_interop.hpp"
#include <iostream>
#include <imgui.h>
#include <imgui_internal.h>
#include "string_helpers.hpp"
#include "copy_handler.hpp"

vec3f process_colour(vec3f in)
{
    if(ImGui::IsStyleLinearColor())
    {
        return srgb_to_lin(in);
    }

    return in;
}

///this function should handle autocolouring
std::vector<render_string> create_render_strings(std::string_view in, bool include_specials)
{
    std::vector<render_string> ret;

    if(in.size() == 0)
        return ret;

    vec3f default_colour = process_colour(letter_to_colour('A').value());

    render_string current_chunk;
    current_chunk.colour = default_colour;

    bool currently_colouring = false;
    bool set_colour = false;

    auto bump_colour = [&]()
    {
        ///else, reset current chunk. Resetting current chunk is a no op, because the length is 0 and the start is valid
        ///the colour always gets overwritten
        if(current_chunk.length > 0)
        {
            ret.push_back(current_chunk);
            render_string& last_chunk = ret.back();

            current_chunk = render_string();
            current_chunk.start = last_chunk.start + last_chunk.length;
            current_chunk.length = 0;
        }
    };

    auto add_index = [&](int idx)
    {
        if(idx == current_chunk.start + current_chunk.length)
        {
            current_chunk.length++;
        }
        else
        {
            assert(idx > current_chunk.start + current_chunk.length);

            vec3f last_colour = current_chunk.colour;

            bump_colour();

            current_chunk.colour = last_colour;
            current_chunk.start = idx;
            current_chunk.length = 1;
        }
    };

    int length = in.size();

    for(int i=0; i < length; i++)
    {
        char cur = in[i];
        char next = 0;
        char next_2 = 0;

        if(i < length - 1)
            next = in[i + 1];

        if(i < length - 2)
            next_2 = in[i + 2];

        // \\`
        if(cur == '\\' && next == '\\' && next_2 == '`')
        {
            add_index(i);

            if(include_specials)
            {
                add_index(i+1);
            }

            i++;
            continue;
        }

        // \`
        if(cur == '\\' && next == '`')
        {
            if(include_specials)
            {
                add_index(i);
            }

            add_index(i+1);

            i++;
            continue;
        }

        // ` initial
        if(cur == '`' && !currently_colouring)
        {
            currently_colouring = true;

            vec3f col = process_colour(letter_to_colour(next).value_or(default_colour));

            bump_colour();
            current_chunk.colour = col;

            if(include_specials)
            {
                add_index(i);
                add_index(i+1);
            }

            if(next != '`')
                i++;

            continue;
        }

        if(cur != '`' && cur != '\n')
        {
            add_index(i);
            continue;
        }

        if((cur == '`' && currently_colouring) || cur == '\n')
        {
            if(include_specials)
            {
                add_index(i);
            }
            else
            {
                if(cur != '`')
                {
                    add_index(i);
                }
            }

            bump_colour();
            currently_colouring = false;
            current_chunk.colour = default_colour;
            continue;
        }
    }

    bump_colour();

    return ret;
}

paragraph_string::paragraph_string(){}

paragraph_string::paragraph_string(std::string in, bool include_specials)
{
    basic_render_strings = create_render_strings(in, include_specials);
    str = std::move(in);
}

std::vector<screen_line> create_screen_lines(const std::string& base_string, const std::vector<render_string>& basic_render_strings, float clipping_width)
{
    std::vector<screen_line> ret;

    vec2f character_dim = {char_inf::cwidth, char_inf::cheight};

    vec2f pos = {0,0};

    screen_line next_line;

    for(int i=0; i < (int)basic_render_strings.size(); i++)
    {
        const render_string& current_string = basic_render_strings[i];

        render_string pending;
        pending.start = current_string.start;
        pending.colour = current_string.colour;

        auto bump_next_screen_line = [&]()
        {
            if(pending.length > 0)
            {
                next_line.strings.push_back(pending);

                int next_start = pending.start + pending.length;

                pending.start = next_start;
                pending.length = 0;

                ret.push_back(next_line);

                next_line = screen_line();
            }
        };

        for(int kk=0; kk < current_string.length; kk++)
        {
            int idx = kk + current_string.start;

            char c = base_string[idx];

            pending.length++;

            if(pos.x() >= clipping_width || c == '\n')
            {
                pos.y() += character_dim.y();
                pos.x() = 0;

                bump_next_screen_line();
            }

            if(c == '\n')
                continue;

            pos.x() += character_dim.x();
        }

        if(pending.length > 0)
        {
            next_line.strings.push_back(pending);
        }
    }

    if(next_line.strings.size() > 0)
    {
        ret.push_back(next_line);
    }

    return ret;
}

void paragraph_string::build(float clip_width)
{
    dim.x() = clip_width;

    lines = create_screen_lines(str, basic_render_strings, clip_width);

    dim.y() = lines.size() * char_inf::cheight;
}

void text_manager::add_main_text(std::string str)
{
    paragraphs.emplace_back(std::move(str), false);
}

void text_manager::relayout(vec2f new_window_size)
{
    vec2f old_window_size = window_size;
    window_size = new_window_size;

    if(old_window_size != new_window_size)
    {
        if(scrollbar_at_bottom)
        {
            should_reset_scrollbar = true;
        }

        printf("Relay\n");
    }

    if(old_window_size.x() == new_window_size.x() && cached_character_size == (vec2f){char_inf::cwidth, char_inf::cheight})
        return;

    cached_character_size = {char_inf::cwidth, char_inf::cheight};

    for(paragraph_string& s : paragraphs)
    {
        s.build(new_window_size.x() - 2 * char_inf::cwbuf);
    }

    if(scrollbar_at_bottom)
    {
        should_reset_scrollbar = true;
    }
}

void text_manager::render()
{
    float clip_width = window_size.x() - 2 * char_inf::cwbuf;
    float content_height = 0;

    for(const paragraph_string& s : paragraphs)
    {
        content_height += s.lines.size() * char_inf::cheight;
    }

    ImGui::SetNextWindowContentSize({clip_width, content_height});
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Appearing);

    ImGui::Begin("Test Terminal");

    copy_handler2& handle = get_global_copy_handler2();

    std::string copy_string;

    bool check_copy = false;
    bool trigger_copy = false;

    if(ImGui::IsWindowFocused())
    {
        if(handle.should_initiate_copy())
        {
            check_copy = true;
            trigger_copy = true;
        }

        if(handle.is_dragging())
        {
            check_copy = true;
        }
    }

    ///cancel copying if the titlebar is hovered
    if(ImGui::IsItemHovered() && !handle.is_dragging())
    {
        handle.cancelled = true;
    }

    vec2f found_window_size = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};

    if(found_window_size != window_size)
    {
        handle.cancelled = true;
    }

    if(handle.cancelled)
    {
        check_copy = false;
    }

    float window_padding_y = ImGui::GetStyle().WindowPadding.y;

    if(has_rendered_once)
    {
        if(scrollbar_at_bottom && found_window_size != window_size)
        {
            printf("Window resized\n");
            should_reset_scrollbar = true;
        }

        if(should_reset_scrollbar)
            scrollbar_at_bottom = true;

        if(should_reset_scrollbar)
        {
            ImGui::SetScrollY(ImGui::GetScrollMaxY());
            printf("Vals %f %f\n", ImGui::GetScrollY(), ImGui::GetScrollMaxY());
        }

        should_reset_scrollbar = false;
    }

    float scroll_y = ImGui::GetScrollY();
    float max_scroll_y = ImGui::GetScrollMaxY();

    float scroll_fraction = 1;

    if(max_scroll_y != 0)
    {
        scroll_fraction = scroll_y / max_scroll_y;
    }

    float adjusted_scroll_fraction = scroll_fraction;

    if(content_height > 0)
    {
        ///so, when scroll_fraction is 1, we want visible_y_end to be lines.size() * size + padding

        float desired_visible_y_end = content_height + char_inf::cheight * 2 + window_padding_y;

        ///vye = scroll_fraction * content_height + window_size.y()
        ///(vye - window_size.y()) / content_height = scroll_fraction

        float scroll_fraction_at_end = (desired_visible_y_end - window_size.y()) / content_height;

        adjusted_scroll_fraction = mix(0, scroll_fraction_at_end, scroll_fraction);
    }

    if(has_rendered_once)
    {
        if(scroll_fraction < 1)
            scrollbar_at_bottom = false;

        if(scroll_fraction == 1)
            scrollbar_at_bottom = true;

        printf("Scroll frac %f\n", scroll_fraction);
    }

    ///in pixels
    float visible_y_start = adjusted_scroll_fraction * content_height;
    float visible_y_end = visible_y_start + window_size.y();

    float current_pixel_y = 0;

    float base_left_offset = char_inf::cwbuf + ImGui::GetWindowPos().x;
    float base_top_offset = ImGui::GetWindowPos().y;

    float decoration_up_height = ImGui::GetCurrentWindow()->TitleBarHeight() + ImGui::GetCurrentWindow()->MenuBarHeight();
    float title_offset = decoration_up_height + window_padding_y;

    vec2f cdim = {char_inf::cwidth, char_inf::cheight};

    vec3f srgb_selection_colour = {80, 80, 255};
    vec3f selection_colour = srgb_to_lin(srgb_selection_colour / 255.f) * 255.f;
    ImU32 selection_colour_u32 = IM_COL32((int)selection_colour.x(), (int)selection_colour.y(), (int)selection_colour.z(), 255);

    vec3f selection_light = srgb_to_lin(srgb_selection_colour / 255.f) * 255.f;
    ImU32 selection_light_u32 = IM_COL32((int)selection_light.x(), (int)selection_light.y(), (int)selection_light.z(), 128);

    vec2f highlight_tl = {FLT_MAX, FLT_MAX};
    vec2f highlight_br = {-FLT_MAX, -FLT_MAX};
    bool any_highlighted = false;

    ///step 1: render everything
    ///step 2: render only stuff in visible region
    ///step 3: remove the content height calculation above
    for(const paragraph_string& s : paragraphs)
    {
        for(const screen_line& sl : s.lines)
        {
            float left_offset = base_left_offset;

            float top_offset = current_pixel_y;

            if(top_offset >= visible_y_start - char_inf::cheight && top_offset < visible_y_end + char_inf::cheight)
            {
                float from_top_of_window = top_offset - visible_y_start;

                float padded_y = from_top_of_window + base_top_offset + title_offset;

                for(const render_string& rs : sl.strings)
                {
                    vec3f colour = rs.colour;

                    int idx_start = rs.start;
                    int idx_len = rs.length;

                    int ir = colour.x() * 255;
                    int ig = colour.y() * 255;
                    int ib = colour.z() * 255;

                    const char* start = s.str.c_str() + idx_start;
                    const char* fin = s.str.c_str() + idx_start + idx_len;

                    ImDrawList* imlist = ImGui::GetWindowDrawList();

                    imlist->AddText(ImVec2(left_offset, padded_y), IM_COL32(ir, ig, ib, 255), start, fin);

                    if(check_copy || trigger_copy)
                    {
                        for(int kk=rs.start; kk < rs.start + rs.length; kk++)
                        {
                            char c = s.str[kk];

                            vec2f pos = {left_offset + (kk - rs.start) * char_inf::cwidth, padded_y};

                            if(handle.char_within_region(pos, cdim))
                            {
                                if(trigger_copy)
                                {
                                    if(copy_string.size() != 0 && handle.last_copy_y != pos.y())
                                        copy_string += "\n" + std::string(1, c);
                                    else
                                        copy_string += std::string(1, c);

                                    handle.last_copy_y = pos.y();
                                }

                                highlight_tl = min(highlight_tl, pos);
                                highlight_br = max(highlight_br, pos + cdim);

                                any_highlighted = true;
                            }
                        }
                    }

                    left_offset += rs.length * char_inf::cwidth;
                }
            }

            current_pixel_y += char_inf::cheight;
        }
    }

    if(any_highlighted)
    {
        ImDrawList* imlist = ImGui::GetWindowDrawList();

        ImVec2 tl = {highlight_tl.x(), highlight_tl.y()};
        ImVec2 br = {highlight_br.x(), highlight_br.y()};

        int thick = 0;

        ImVec2 ptl = {tl.x + thick, tl.y + thick};
        ImVec2 pbr = {br.x - thick, br.y - thick};

        //selection_light_u32
        imlist->AddRectFilled(ptl, pbr, selection_light_u32, 0, 0);
        imlist->AddRect(tl, br, selection_colour_u32, 1.f, 0, 2);
    }

    if(trigger_copy)
    {
        if(copy_string.size() > 0)
        {
            std::cout << "Copied2 " << copy_string << std::endl;

            handle.set_clipboard(copy_string);
        }

        handle.reset_trigger();
    }

    ImGui::End();

    has_rendered_once = true;

    relayout(found_window_size);
}

void test_render_strings()
{
    std::string base = "hello there []\" asdf `Xcatepillar`\n`B`uncoloured\n`Dhithere\n``uncoloured`Dcoloured1`randomtext`Bcoloured2\n";

    std::vector<render_string> strs = create_render_strings(base, false);

    for(render_string& rstr : strs)
    {
        std::string_view view(base.begin() + rstr.start, base.begin() + rstr.start + rstr.length);

        std::cout << view << "|";
    }

    std::cout << "----\n";

    std::vector<screen_line> screen_lines = create_screen_lines(base, strs, 60);

    for(screen_line& line : screen_lines)
    {
        for(render_string& rstr : line.strings)
        {
            std::string_view view(base.begin() + rstr.start, base.begin() + rstr.start + rstr.length);

            std::cout << view << "|";
        }

        std::cout << "-\n";
    }
}
