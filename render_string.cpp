#include "render_string.hpp"
#include "colour_interop.hpp"
#include <iostream>
#include <imgui.h>
#include <imgui_internal.h>
#include "string_helpers.hpp"
#include "copy_handler.hpp"
#include "auto_handlers.hpp"
#include <toolkit/fs_helpers.hpp>
#include <GLFW/glfw3.h>
#include <codecvt>
#include <locale>
#include <toolkit/clipboard.hpp>

vec3f process_colour(vec3f in)
{
    if(ImGui::IsStyleLinearColor())
    {
        return srgb_to_lin(in/255.f)*255.f;
    }

    return in;
}

///this function should handle autocolouring
std::vector<render_string> create_render_strings(std::string_view in, bool include_specials, bool colour_like_terminal)
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
            if(!currently_colouring)
            {
                int chunk_start = current_chunk.start;

                std::string_view as_view = std::string_view(in.begin() + current_chunk.start, in.begin() + current_chunk.start + current_chunk.length);

                std::vector<render_string> autocoloured = auto_colour(as_view, colour_like_terminal);

                for(render_string& strs : autocoloured)
                {
                    strs.start += chunk_start;
                }

                ret.insert(ret.end(), autocoloured.begin(), autocoloured.end());
            }
            else
            {
                ret.push_back(current_chunk);
            }

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

            if(currently_colouring)
                current_chunk.colour = last_colour;
            else
                current_chunk.colour = default_colour;

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

paragraph_string::paragraph_string(std::string in, bool include_specials, bool colour_like_terminal)
{
    basic_render_strings = create_render_strings(in, include_specials, colour_like_terminal);
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

void text_manager::add_main_text(std::string str, auto_handler& auto_handle)
{
    paragraphs.emplace_back(std::move(str), false, colour_like_terminal);

    std::vector<std::string> autos = parse_for_autocompletes(str);

    for(std::string& s : autos)
    {
        if(auto_handle.found_args.find(s) == auto_handle.found_args.end())
        {
            auto_handle.found_unprocessed_autocompletes.push_back(std::move(s));
        }
    }

    unseen_text = !was_visible;
}

void text_manager::add_main_text(std::string str)
{
    paragraphs.emplace_back(std::move(str), false, colour_like_terminal);

    unseen_text = !was_visible;
}

void text_manager::add_command_to_main_text(auto_handler& auto_handle, connection_send_data& send)
{
    on_enter_text(command.command, send);

    add_main_text(std::move(command.command), auto_handle);
    command.push_command_to_history(command.command);
    command.clear_command();
}

float get_formatting_clip_width(float new_window_width, float scrollbar_width)
{
    return new_window_width - 2 * char_inf::cwbuf - scrollbar_width - ImGui::GetStyle().FramePadding.x - 2 * char_inf::cwidth;
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
    }

    if(old_window_size.x() == new_window_size.x() && cached_character_size == (vec2f){char_inf::cwidth, char_inf::cheight})
        return;

    cached_character_size = {char_inf::cwidth, char_inf::cheight};

    for(paragraph_string& s : paragraphs)
    {
        s.build(get_formatting_clip_width(new_window_size.x(), scrollbar.width));
    }

    if(scrollbar_at_bottom)
    {
        should_reset_scrollbar = true;
    }
}

float get_window_title_offset()
{
    float decoration_up_height = ImGui::GetCurrentWindow()->TitleBarHeight() + ImGui::GetCurrentWindow()->MenuBarHeight();
    float title_offset = decoration_up_height + ImGui::GetStyle().WindowPadding.y;

    return title_offset;
}

void driven_scrollbar::render(int trailing_blank_lines)
{
    float paddingx = ImGui::GetStyle().FramePadding.x;
    float paddingy = ImGui::GetStyle().FramePadding.y + 16;

    float height = ImGui::GetWindowSize().y;

    float render_height = (height - paddingy) - get_window_title_offset();

    if(ImGui::IsWindowFocused())
    {
        if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_PageDown]))
        {
            adjust_by_lines(floor(render_height / char_inf::cheight), trailing_blank_lines);
        }

        if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_PageUp]))
        {
            adjust_by_lines(-floor(render_height / char_inf::cheight), trailing_blank_lines);
        }
    }

    if(ImGui::IsWindowHovered())
    {
        if(ImGui::GetIO().MouseWheel != 0)
        {
            adjust_by_lines(-ImGui::GetIO().MouseWheel, trailing_blank_lines);
        }
    }

    float render_x = ImGui::GetWindowSize().x - width - paddingx;
    float render_y = get_window_title_offset();

    ImDrawList* imlist = ImGui::GetWindowDrawList();

    ImVec2 window_pos = ImGui::GetWindowPos();

    ImU32 col = IM_COL32(5, 5, 5, 255);
    ImU32 col2 = IM_COL32(2, 2, 2, 255);

    ImVec2 tl = {render_x, render_y};
    ImVec2 br = {render_x + width, height - paddingy};

    float scroll_height = br.y - tl.y;

    tl.x += window_pos.x;
    tl.y += window_pos.y;

    br.x += window_pos.x;
    br.y += window_pos.y;

    imlist->AddRectFilled(tl, br, col2, 0, 0);
    imlist->AddRect(tl, br, col, 0, 0, 1);

    ImGui::SetCursorScreenPos(tl);

    ImGui::InvisibleButton("InvisiB", ImVec2(br.x - tl.x, br.y - tl.y), 0);

    float scrollbar_height = 14;

    float desired_scrollbar_end = scroll_height;

    float scroll_fraction_at_end = (desired_scrollbar_end - scrollbar_height) / scroll_height;

    float adjusted_scroll_fraction = mix(0, scroll_fraction_at_end, fraction);

    float position_y_top = adjusted_scroll_fraction * scroll_height;
    float position_y_bottom = position_y_top + scrollbar_height;

    ImU32 col_bar = IM_COL32(15, 15, 15, 255);

    float sx = 2;

    if(ImGui::IsItemHovered())
    {
        col_bar = IM_COL32(50, 50, 50, 255);
    }

    imlist->AddRectFilled({tl.x + sx, position_y_top + 2 + render_y + window_pos.y}, {br.x - sx, position_y_bottom + render_y + window_pos.y - 2}, col_bar, 6, 0);

    if(ImGui::IsItemClicked() || ImGui::IsItemActive())
    {
        float mouse_y = ImGui::GetMousePos().y;

        float mouse_position_at_0 = tl.y + scrollbar_height/2.f;
        float mouse_position_at_1 = tl.y + scroll_height - scrollbar_height/2.f;

        float mouse_y_fraction = (mouse_y - mouse_position_at_0) / (mouse_position_at_1 - mouse_position_at_0);

        fraction = mouse_y_fraction;

        fraction = clamp(fraction, 0.f, 1.f);
    }
}

void driven_scrollbar::adjust_by_px(float py)
{
    float adjust_pixels = py;

    float as_frac = adjust_pixels / content_height;

    fraction += as_frac;

    fraction = clamp(fraction, 0.f, 1.f);
}

float get_desired_visible_y_end(float content_height, int trailing_blank_lines)
{
    int additional = trailing_blank_lines;

    return content_height + char_inf::cheight * (2 + additional) + ImGui::GetStyle().WindowPadding.y;
}

void driven_scrollbar::adjust_by_lines(float lines, int trailing_blank_lines)
{
    float desired_visible_y_end = get_desired_visible_y_end(content_height, trailing_blank_lines);

    float scroll_fraction_at_end = (desired_visible_y_end - window_size.y()) / content_height;

    ///vys = asf * ch + x

    ///dvye = ch + constant
    ///sfae =

    ///so asf = sfae * scroll_fraction
    ///and vys = asf * ch

    ///sfae * (scroll_fraction + x) * ch = vys
    ///sfae * scroll_fraction + x * sfae * ch = vys

    float adjust_pixels = lines * char_inf::cheight / scroll_fraction_at_end;

    float as_frac = adjust_pixels / content_height;

    fraction += as_frac;

    fraction = clamp(fraction, 0.f, 1.f);
}

void text_manager::default_controls(auto_handler& auto_handle, connection_send_data& send)
{
    if(!was_focused)
        return;

    ImGuiIO& io = ImGui::GetIO();

    std::vector<uint32_t> input_utf32;

    for(auto& i : io.InputQueueCharacters)
    {
        input_utf32.push_back(i);
    }

    for(uint32_t i : input_utf32)
    {
        if(i <= 126 && i >= 32)
        {
            std::u32string utf32;
            utf32.push_back(i);

            std::string utf8;

            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;

            utf8 = cvt.to_bytes(utf32);

            command.add_to_command(i);
        }
    }

    bool enter = false;

    int key_count = 512;

    for(int i=0; i < key_count; i++)
    {
        if(!ImGui::IsKeyPressed(i))
            continue;

        if(i == io.KeyMap[ImGuiKey_Backspace])
        {
            if(io.KeyCtrl)
            {
                for(int kk=0; kk < 5; kk++)
                    command.process_backspace();
            }
            else
            {
                command.process_backspace();
            }
        }

        if(i == io.KeyMap[ImGuiKey_Delete])
        {
            if(io.KeyCtrl)
            {
                for(int kk=0; kk < 5; kk++)
                    command.process_delete();
            }
            else
            {
                command.process_delete();
            }
        }

        if(i == io.KeyMap[ImGuiKey_UpArrow])
        {
            command.move_command_history_idx(-1);
        }

        if(i == io.KeyMap[ImGuiKey_DownArrow])
        {
            command.move_command_history_idx(1);
        }

        if(i == io.KeyMap[ImGuiKey_LeftArrow])
        {
            if(!io.KeyCtrl)
                command.move_cursor(-1);
            else
                command.move_cursor(-5);
        }

        if(i == io.KeyMap[ImGuiKey_RightArrow])
        {
            if(!io.KeyCtrl)
                command.move_cursor(1);
            else
                command.move_cursor(5);
        }

        if(i == io.KeyMap[ImGuiKey_Home])
        {
            command.move_cursor(-(int)command.command.size());
        }

        if(i == io.KeyMap[ImGuiKey_End])
        {
            command.move_cursor(command.command.size());
        }

        if(i == io.KeyMap[ImGuiKey_Escape])
        {
            command.clear_command();
        }

        if(i == io.KeyMap[ImGuiKey_V])
        {
            if(io.KeyCtrl)
            {
                std::string add_text = clipboard::get();

                for(auto& c : add_text)
                {
                    command.add_to_command(c);
                }
            }
        }

        if(i == io.KeyMap[ImGuiKey_Enter] || i == io.KeyMap[ImGuiKey_KeyPadEnter])
        {
            if(!io.KeyCtrl && !io.KeyShift)
                enter = true;
            else
                command.add_to_command('\n');
        }
    }

    int mouse_buttons = 5;

    for(int i=0; i < mouse_buttons; i++)
    {
        if(!ImGui::IsMouseClicked(i))
            continue;

        if(i == 1)
        {
            std::string add_text = clipboard::get();

            for(auto& c : add_text)
            {
                command.add_to_command(c);
            }
        }
    }

    if(enter)
    {
        add_command_to_main_text(auto_handle, send);
    }
}

bool text_manager::create_window(vec2f content_size, vec2f create_window_size)
{
    ImGui::SetNextWindowContentSize({content_size.x(), content_size.y()});
    ImGui::SetNextWindowSize(ImVec2(create_window_size.x(), create_window_size.y()), ImGuiCond_Appearing);

    int flags = ImGuiWindowFlags_NoScrollbar;

    if(unseen_text)
        flags |= ImGuiWindowFlags_UnsavedDocument;

    return ImGui::Begin("Test Terminal", &open, flags);
}

void text_manager::render()
{
    float clip_width = window_size.x() - 2 * char_inf::cwbuf;
    float content_height = 0;

    for(const paragraph_string& s : paragraphs)
    {
        content_height += s.lines.size() * char_inf::cheight;
    }

    ///ImGui::Begin
    bool should_render = create_window({clip_width, content_height}, {400, 300});

    vec2f found_window_size = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};

    dock_id = ImGui::GetWindowDockID();
    was_focused = ImGui::IsWindowFocused();

    if(should_render)
    {
        if(should_reset_scrollbar)
            scrollbar.fraction = 1;

        should_reset_scrollbar = false;

        int trailing_blank_lines = 0;

        paragraph_string command_line(command_visual_prefix + command.command, true, true);
        command_line.build(get_formatting_clip_width(window_size.x(), scrollbar.width));

        int command_line_height = command_line.lines.size();

        if(command_line_height == 0)
            trailing_blank_lines = 1;

        trailing_blank_lines += command_line_height;

        scrollbar.content_height = content_height;
        scrollbar.window_size = window_size;

        scrollbar.render(trailing_blank_lines);
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

        if(ImGui::IsWindowHovered() && !handle.is_dragging())
        {
            vec2f tl = {ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + get_window_title_offset()};
            vec2f br = {ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - scrollbar.width - char_inf::cwbuf - ImGui::GetStyle().FramePadding.x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y};

            vec2f mouse_pos = {ImGui::GetMousePos().x, ImGui::GetMousePos().y};

            bool point_in_rect = mouse_pos.x() >= tl.x() && mouse_pos.x() < br.x() && mouse_pos.y() >= tl.y() && mouse_pos.y() < br.y();

            if(!point_in_rect)
            {
                handle.cancelled = true;
            }
        }

        if(found_window_size != window_size)
        {
            handle.cancelled = true;
        }

        if(handle.cancelled)
        {
            check_copy = false;
        }

        float scroll_fraction = scrollbar.fraction;

        float adjusted_scroll_fraction = scroll_fraction;

        if(content_height > 0)
        {
            ///so, when scroll_fraction is 1, we want visible_y_end to be lines.size() * size + padding
            float desired_visible_y_end = get_desired_visible_y_end(content_height, trailing_blank_lines);

            ///vye = scroll_fraction * content_height + window_size.y()
            ///(vye - window_size.y()) / content_height = scroll_fraction

            float scroll_fraction_at_end = (desired_visible_y_end - window_size.y()) / content_height;

            adjusted_scroll_fraction = scroll_fraction_at_end * scroll_fraction;
        }

        if(scroll_fraction < 1)
            scrollbar_at_bottom = false;

        if(scroll_fraction == 1)
            scrollbar_at_bottom = true;

        ///in pixels
        float visible_y_start = adjusted_scroll_fraction * content_height;
        float visible_y_end = visible_y_start + window_size.y();

        float current_pixel_y = 0;

        float base_left_offset = char_inf::cwbuf + ImGui::GetWindowPos().x;
        float base_top_offset = ImGui::GetWindowPos().y;

        float title_offset = get_window_title_offset();

        vec2f cdim = {char_inf::cwidth, char_inf::cheight};

        vec3f srgb_selection_colour = {80, 80, 255};
        vec3f selection_colour = srgb_to_lin(srgb_selection_colour / 255.f) * 255.f;
        ImU32 selection_colour_u32 = IM_COL32((int)selection_colour.x(), (int)selection_colour.y(), (int)selection_colour.z(), 255);

        vec3f selection_light = srgb_to_lin(srgb_selection_colour / 255.f) * 255.f;
        ImU32 selection_light_u32 = IM_COL32((int)selection_light.x(), (int)selection_light.y(), (int)selection_light.z(), 128);

        vec2f highlight_tl = {FLT_MAX, FLT_MAX};
        vec2f highlight_br = {-FLT_MAX, -FLT_MAX};
        bool any_highlighted = false;

        auto process_screen_line = [&](const paragraph_string& s, const screen_line& sl, float y_screen)
        {
            float left_offset = base_left_offset;

            for(const render_string& rs : sl.strings)
            {
                vec3f colour = rs.colour;

                int idx_start = rs.start;
                int idx_len = rs.length;

                int ir = colour.x();
                int ig = colour.y();
                int ib = colour.z();

                const char* start = s.str.c_str() + idx_start;
                const char* fin = s.str.c_str() + idx_start + idx_len;

                ImDrawList* imlist = ImGui::GetWindowDrawList();

                imlist->AddText(ImVec2(left_offset, y_screen), IM_COL32(ir, ig, ib, 255), start, fin);

                if(check_copy || trigger_copy)
                {
                    for(int kk=rs.start; kk < rs.start + rs.length; kk++)
                    {
                        char c = s.str[kk];

                        vec2f pos = {left_offset + (kk - rs.start) * char_inf::cwidth, y_screen};

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
        };

        auto process_paragraph = [&](const paragraph_string& s)
        {
            for(const screen_line& sl : s.lines)
            {
                float top_offset = current_pixel_y;

                float from_top_of_window = top_offset - visible_y_start;

                float padded_y = from_top_of_window + base_top_offset + title_offset;

                if(top_offset >= visible_y_start - char_inf::cheight && (top_offset < visible_y_end - (3.5 + trailing_blank_lines) * char_inf::cheight))
                {
                    process_screen_line(s, sl, padded_y);
                }

                current_pixel_y += char_inf::cheight;
            }
        };

        auto process_paragraph_with_y = [&](const paragraph_string& s, float screen_y)
        {
            for(const screen_line& sl : s.lines)
            {
                process_screen_line(s, sl, screen_y);

                screen_y += char_inf::cheight;
            }
        };

        ///step 1: render everything
        ///step 2: render only stuff in visible region
        ///step 3: remove the content height calculation above
        for(const paragraph_string& s : paragraphs)
        {
            process_paragraph(s);
        }

        {
            float screen_y = base_top_offset + window_size.y() - ImGui::GetStyle().WindowPadding.y - char_inf::cheight * trailing_blank_lines;

            process_paragraph_with_y(command_line, screen_y);
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
    }

    ImGui::End();

    if(should_render)
    {
        relayout(found_window_size);
        unseen_text = false;
    }

    was_visible = should_render;
}

void text_manager::clear_text()
{
    paragraphs.clear();
}

void text_manager::clear_command()
{
    command.clear_command();
}

main_terminal2::main_terminal2()
{
    colour_like_terminal = true;
    command.command = "user ";
    command.cursor_pos_idx = command.command.size();
}

child_terminal::child_terminal()
{
    colour_like_terminal = true;
}

void chat_manager::set_chat_channels(const std::vector<std::string>& channels)
{
    open_chat_channels = channels;
}

void chat_manager::add_text(const std::string& channel, const std::vector<std::string>& text)
{
    for(const std::string& str : text)
    {
        chat_threads[channel].add_main_text(str);
    }
}

bool chat_thread2::create_window(vec2f content_size, vec2f create_window_size)
{
    create_window_size = {500, 300};

    ImGui::SetNextWindowContentSize({content_size.x(), content_size.y()});
    ImGui::SetNextWindowSize(ImVec2(create_window_size.x(), create_window_size.y()), ImGuiCond_Appearing);

    int flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;

    if(unseen_text)
        flags |= ImGuiWindowFlags_UnsavedDocument;

    return ImGui::Begin("Chat Thread", &open, flags);
}

void chat_manager::render()
{
    static bool once = file::exists("ui_setup_once_v2");
    static ImGuiID dock_id = -1;

    if(!once || ImGui::IsKeyPressed(GLFW_KEY_F2))
    {
        dock_id = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
        ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;
        ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
        ImGui::DockBuilderSetNodePos(dock_id, ImVec2(viewport_pos.x + viewport_size.x - 600, viewport_pos.y + 100));
        ImGui::DockBuilderSetNodeSize(dock_id, ImVec2(500, 300));

        for(const std::string& channel : open_chat_channels)
        {
            ImGui::DockBuilderDockWindow(("###" + channel).c_str(), dock_id);
        }

        ImGui::DockBuilderFinish(dock_id);

        file::write("ui_setup_once_v2", "1", file::mode::BINARY);
    }

    std::map<int, int> dock_ids;

    for(const std::string& channel_name : open_chat_channels)
    {
        chat_thread2& thread = chat_threads[channel_name];

        ImGui::SetNextWindowDockID(dock_id, ImGuiCond_FirstUseEver);

        thread.name = channel_name + "###" + channel_name;

        thread.render();

        ///should this be done unconditionally?
        //if(thread.was_visible)
        {
            if(dock_id == (ImGuiID)-1)
            {
                dock_ids[thread.dock_id]++;
            }
        }
    }

    ///this takes the dock id with the largest number of chat windows docked in it
    ///and makes that the default docking id
    if(dock_id == (ImGuiID)-1)
    {
        int greatest_id = -1;
        int greatest_count = 0;

        for(auto& i : dock_ids)
        {
            if(i.second > greatest_count)
            {
                greatest_count = i.second;
                greatest_id = i.first;
            }
        }

        if(greatest_id != -1)
        {
            dock_id = greatest_id;
        }
        else
        {
            once = false;
        }
    }

    once = true;
}

void test_render_strings()
{
    std::string base = "hello there []\" asdf `Xcatepillar`\n`B`uncoloured\n`Dhithere\n``uncoloured`Dcoloured1`randomtext`Bcoloured2\n";

    auto_handler handle;

    std::vector<render_string> strs = create_render_strings(base, false, true);

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
