#include "render_string.hpp"
#include "colour_interop.hpp"
#include <iostream>
#include <imgui.h>
#include <imgui_internal.h>
#include "string_helpers.hpp"

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
    if(window_size == new_window_size && cached_character_size == (vec2f){char_inf::cwidth, char_inf::cheight})
        return;

    window_size = new_window_size;
    cached_character_size = {char_inf::cwidth, char_inf::cheight};

    for(paragraph_string& s : paragraphs)
    {
        s.build(new_window_size.x() - 2 * char_inf::cwbuf);
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

    float window_padding_y = ImGui::GetStyle().WindowPadding.y;

    float scroll_y = ImGui::GetScrollY();
    float max_scroll_y = ImGui::GetScrollMaxY();

    float scroll_fraction = 1;

    if(max_scroll_y != 0)
    {
        scroll_fraction = scroll_y / max_scroll_y;
    }

    float adjusted_scroll_fracton = scroll_fraction;

    if(content_height > 0)
    {
        ///so, when scroll_fraction is 1, we want visible_y_end to be lines.size() * size + padding

        float desired_visible_y_end = content_height + char_inf::cheight * 2 + window_padding_y;

        ///vye = scroll_fraction * content_height + window_size.y()
        ///(vye - window_size.y()) / content_height = scroll_fraction

        float scroll_fraction_at_end = (desired_visible_y_end - window_size.y()) / content_height;

        adjusted_scroll_fracton = mix(0, scroll_fraction_at_end, scroll_fraction);
    }

    ///in pixels
    float visible_y_start = adjusted_scroll_fracton * content_height;
    float visible_y_end = visible_y_start + window_size.y();

    float current_pixel_y = 0;

    float base_left_offset = char_inf::cwbuf + ImGui::GetWindowPos().x;
    float base_top_offset = ImGui::GetWindowPos().y;
    //float title_offset = ImGui::GetCursorStartPos().y;

    float decoration_up_height = ImGui::GetCurrentWindow()->TitleBarHeight() + ImGui::GetCurrentWindow()->MenuBarHeight();
    float title_offset = decoration_up_height + window_padding_y;

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
                for(const render_string& rs : sl.strings)
                {
                    float from_top_of_window = top_offset - visible_y_start;

                    vec3f colour = rs.colour;

                    int idx_start = rs.start;
                    int idx_len = rs.length;

                    int ir = colour.x() * 255;
                    int ig = colour.y() * 255;
                    int ib = colour.z() * 255;

                    const char* start = s.str.c_str() + idx_start;
                    const char* fin = s.str.c_str() + idx_start + idx_len;

                    ImDrawList* imlist = ImGui::GetWindowDrawList();

                    imlist->AddText(ImVec2(left_offset, from_top_of_window + base_top_offset + title_offset), IM_COL32(ir, ig, ib, 255), start, fin);

                    left_offset += rs.length * char_inf::cwidth;
                }
            }

            current_pixel_y += char_inf::cheight;
        }
    }

    vec2f found_window_size = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};

    ImGui::End();

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
