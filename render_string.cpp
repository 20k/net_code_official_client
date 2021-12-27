#include "render_string.hpp"
#include "colour_interop.hpp"
#include <iostream>
#include <imgui.h>
#include <imgui_internal.h>
#include "copy_handler.hpp"
#include "auto_handlers.hpp"
#include <toolkit/fs_helpers.hpp>
#include <GLFW/glfw3.h>
#include <codecvt>
#include <locale>
#include <toolkit/clipboard.hpp>
#include "script_transfer.hpp"
#include "local_commands.hpp"
#include <libncclient/nc_util.hpp>
#include "font_cfg.hpp"
#include <toolkit/render_window.hpp>
#include "context.hpp"

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
    bool suppress_specials = false;

    auto add_render_string = [&](const render_string& str)
    {
        if(str.length == 0)
            return;

        if(ret.size() == 0)
        {
            ret.push_back(str);
        }
        else
        {
            render_string& last = ret.back();

            ///if our start is the same as last's end, and we're the same colour, merge
            if(str.start == (last.start + last.length) && str.colour == last.colour)
            {
                last.length += str.length;
            }
            else
            {
                ret.push_back(str);
            }
        }
    };

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
                    assert(strs.start + strs.length <= current_chunk.start + current_chunk.length);
                }

                for(render_string& strs : autocoloured)
                {
                    strs.start += chunk_start;
                }

                for(render_string& strs : autocoloured)
                {
                    add_render_string(strs);
                }
            }
            else
            {
                add_render_string(current_chunk);
            }

            current_chunk = render_string();

            if(ret.size() > 0)
            {
                render_string& last_chunk = ret.back();
                current_chunk.start = last_chunk.start + last_chunk.length;

                current_chunk.length = 0;
            }
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
            if(next == '|')
                suppress_specials = true;

            bump_colour();

            currently_colouring = true;

            vec3f col = process_colour(letter_to_colour(next).value_or(default_colour));

            current_chunk.colour = col;

            if(include_specials && !suppress_specials)
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
            if(include_specials && !suppress_specials)
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
            suppress_specials = false;
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

vec2f get_char_size(ImFont* font)
{
    if(font == nullptr)
        font = ImGui::GetCurrentContext()->Font;

    float font_size = font->FontSize;

    return xy_to_vec(font->CalcTextSizeA(font_size, FLT_MAX, -1.f, "A", nullptr, nullptr));

    //return xy_to_vec(ImGui::CalcTextSize("A"));
}

std::vector<screen_line> create_screen_lines(ImFont* font, const std::string& base_string, const std::vector<render_string>& basic_render_strings, float clipping_width)
{
    std::vector<screen_line> ret;

    vec2f character_dim = get_char_size(font);

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

        next_line.strings.push_back(pending);
    }

    if(next_line.strings.size() > 0)
    {
        ret.push_back(next_line);
    }

    return ret;
}

void paragraph_string::build(ImFont* font, float clip_width)
{
    dim.x() = clip_width;

    lines = create_screen_lines(font, str, basic_render_strings, clip_width);

    dim.y() = lines.size() * get_char_size(font).y();
}

float get_formatting_clip_width(ImFont* font, float new_window_width, float scrollbar_width)
{
    return new_window_width - 2 * char_inf::cwbuf - scrollbar_width - ImGui::GetStyle().FramePadding.x - 2 * get_char_size(font).x();
}

void text_manager::add_main_text(std::string str, auto_handler& auto_handle)
{
    std::vector<std::string> autos = parse_for_autocompletes(str);

    paragraphs.emplace_back(std::move(str), false, colour_like_terminal);
    paragraphs.back().build(font, get_formatting_clip_width(font, window_size.x(), scrollbar.width));

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
    paragraphs.back().build(font, get_formatting_clip_width(font, window_size.x(), scrollbar.width));

    unseen_text = !was_visible;
}

void text_manager::add_command_to_main_text(auto_handler& auto_handle)
{
    std::string str = command.command;

    add_main_text(str, auto_handle);
    command.push_command_to_history(str);
    command.clear_command();
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

    if(old_window_size.x() == new_window_size.x() && cached_character_size == get_char_size(font))
        return;

    cached_character_size = get_char_size(font);

    for(paragraph_string& s : paragraphs)
    {
        s.build(font, get_formatting_clip_width(font, new_window_size.x(), scrollbar.width));
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

void driven_scrollbar::render(ImFont* font, int trailing_blank_lines)
{
    float paddingx = ImGui::GetStyle().FramePadding.x;
    float paddingy = ImGui::GetStyle().FramePadding.y + 16;

    float height = ImGui::GetWindowSize().y;

    float render_height = (height - paddingy) - get_window_title_offset();

    vec2f char_size = get_char_size(font);

    if(ImGui::IsWindowFocused())
    {
        if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_PageDown]))
        {
            adjust_by_lines(font, floor(render_height / char_size.y()), trailing_blank_lines);
        }

        if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_PageUp]))
        {
            adjust_by_lines(font, -floor(render_height / char_size.y()), trailing_blank_lines);
        }
    }

    if(ImGui::IsWindowHovered())
    {
        if(ImGui::GetIO().MouseWheel != 0)
        {
            adjust_by_lines(font, -ImGui::GetIO().MouseWheel, trailing_blank_lines);
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

    scroll_height = max(scroll_height, 1.f);

    tl.x += window_pos.x;
    tl.y += window_pos.y;

    br.x += window_pos.x;
    br.y += window_pos.y;

    imlist->AddRectFilled(tl, br, col2, 0, 0);
    imlist->AddRect(tl, br, col, 0, 0, 1);

    ImGui::SetCursorScreenPos(tl);

    ImGui::InvisibleButton("InvisiB", ImVec2(max(br.x - tl.x, 4.f), max(br.y - tl.y, 4.f)), 0);

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

    ImVec2 scrollbar_tl = {tl.x + sx, position_y_top + 2 + render_y + window_pos.y};
    ImVec2 scrollbar_br = {br.x - sx, position_y_bottom + render_y + window_pos.y - 2};

    if(scrollbar_br.x - scrollbar_tl.x < 4)
    {
        scrollbar_br.x = scrollbar_tl.x + 4;
    }

    if(scrollbar_br.y - scrollbar_tl.y < 4)
    {
        scrollbar_br.y = scrollbar_tl.y + 4;
    }

    imlist->AddRectFilled(scrollbar_tl, scrollbar_br, col_bar, 6, 0);

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

float get_desired_visible_y_end(ImFont* font, float content_height, int trailing_blank_lines)
{
    int additional = trailing_blank_lines;

    return content_height + get_char_size(font).y() * (2 + additional) + ImGui::GetStyle().WindowPadding.y;
}

void driven_scrollbar::adjust_by_lines(ImFont* font, float lines, int trailing_blank_lines)
{
    float desired_visible_y_end = get_desired_visible_y_end(font, content_height, trailing_blank_lines);

    float scroll_fraction_at_end = (desired_visible_y_end - window_size.y()) / content_height;

    ///vys = asf * ch + x

    ///dvye = ch + constant
    ///sfae =

    ///so asf = sfae * scroll_fraction
    ///and vys = asf * ch

    ///sfae * (scroll_fraction + x) * ch = vys
    ///sfae * scroll_fraction + x * sfae * ch = vys

    float adjust_pixels = lines * get_char_size(font).y() / scroll_fraction_at_end;

    float as_frac = adjust_pixels / content_height;

    fraction += as_frac;

    fraction = clamp(fraction, 0.f, 1.f);
}

void text_manager::default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send)
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

    auto_handle.tab_pressed = ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Tab]);

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
        std::string entry = command.command;

        on_enter_text(ctx, entry, auto_handle, send);
    }
}

bool text_manager::create_window(context& ctx, vec2f content_size, vec2f create_window_size)
{
    ImGui::SetNextWindowContentSize({content_size.x(), content_size.y()});
    ImGui::SetNextWindowSize(ImVec2(create_window_size.x(), create_window_size.y()), ImGuiCond_Appearing);

    int flags = ImGuiWindowFlags_NoScrollbar;

    if(unseen_text)
        flags |= ImGuiWindowFlags_UnsavedDocument;

    return ImGui::Begin("Test Terminal", &open, flags);
}

void text_manager::destroy_window()
{
    ImGui::End();
}

void text_manager::on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send)
{
    add_command_to_main_text(auto_handle);
}

void add_text(ImDrawList* lst, ImFont* font, ImVec2 pos, ImU32 col, const char* start, const char* fin)
{
    if(font == nullptr)
        font = ImGui::GetCurrentContext()->Font;

    float font_size = font->FontSize;

    lst->AddText(font, font_size, pos, col, start, fin);
}

void text_manager::render(context& ctx, auto_handler& auto_handle)
{
    float clip_width = get_formatting_clip_width(font, window_size.x(), scrollbar.width);

    float content_height = 0;

    vec2f char_size = get_char_size(font);

    for(const paragraph_string& s : paragraphs)
    {
        content_height += s.lines.size() * char_size.y();
    }

    ///ImGui::Begin
    bool should_render = create_window(ctx, {clip_width, content_height}, {400, 300});

    vec2f found_window_size = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};

    dock_id = ImGui::GetWindowDockID();
    was_focused = ImGui::IsWindowFocused();

    float base_left_offset = char_inf::cwbuf + ImGui::GetWindowPos().x + ImGui::GetStyle().FramePadding.x;
    float base_top_offset = ImGui::GetWindowPos().y;

    float title_offset = get_window_title_offset();

    window_tl = {base_left_offset, base_top_offset + title_offset};

    if(should_render)
    {
        if(should_reset_scrollbar)
            scrollbar.fraction = 1;

        should_reset_scrollbar = false;

        int trailing_blank_lines = 0;

        std::string render_command = command.command;
        bool specials = true;
        int cursor_offset = 0;

        if(render_command == "")
        {
            render_command = "`bType something here...`";
            specials = false;
        }

        auto_handle.handle_autocompletes(render_command, command.cursor_pos_idx, cursor_offset, command.command);

        render_command = command_visual_prefix + render_command;

        paragraph_string command_line(render_command, specials, true);
        command_line.build(font, get_formatting_clip_width(font, window_size.x(), scrollbar.width));

        int command_line_height = command_line.lines.size();

        if(command_line_height == 0)
            trailing_blank_lines = 1;

        if(!use_type_prompt)
        {
            trailing_blank_lines = 0;
            command_line = paragraph_string();
        }

        trailing_blank_lines += command_line_height;

        scrollbar.content_height = content_height;
        scrollbar.window_size = window_size;

        scrollbar.render(font, trailing_blank_lines);
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
            float desired_visible_y_end = get_desired_visible_y_end(font, content_height, trailing_blank_lines);

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

        vec2f cdim = get_char_size(font);

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

                add_text(imlist, font, ImVec2(left_offset, y_screen), IM_COL32(ir, ig, ib, 255), start, fin);

                if(check_copy || trigger_copy)
                {
                    for(int kk=rs.start; kk < rs.start + rs.length; kk++)
                    {
                        char c = s.str[kk];

                        vec2f pos = {left_offset + (kk - rs.start) * char_size.x(), y_screen};

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

                left_offset += rs.length * char_size.x();
            }
        };

        auto process_paragraph = [&](const paragraph_string& s)
        {
            for(const screen_line& sl : s.lines)
            {
                float top_offset = current_pixel_y;

                float from_top_of_window = top_offset - visible_y_start;

                float padded_y = from_top_of_window + base_top_offset + title_offset;

                if(top_offset >= visible_y_start - char_size.y() && (top_offset < visible_y_end - (3.5 + trailing_blank_lines) * char_size.y()))
                {
                    process_screen_line(s, sl, padded_y);
                }

                current_pixel_y += char_size.y();
            }
        };

        auto process_paragraph_with_y = [&](const paragraph_string& s, float screen_y)
        {
            for(const screen_line& sl : s.lines)
            {
                process_screen_line(s, sl, screen_y);

                screen_y += char_size.y();
            }
        };

        ///step 1: render everything
        ///step 2: render only stuff in visible region
        ///step 3: remove the content height calculation above
        for(const paragraph_string& s : paragraphs)
        {
            process_paragraph(s);
        }

        if(use_type_prompt)
        {
            float screen_y = base_top_offset + window_size.y() - ImGui::GetStyle().WindowPadding.y - char_size.y() * trailing_blank_lines;

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

    destroy_window();

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

std::string on_edit(std::string_view command, std::string_view username)
{
    std::string s_command(command.begin(), command.end());

    std::vector<std::string> fname = no_ss_split(s_command, " ");

    if(fname.size() < 2)
        return make_error_col("Format is #edit scriptname");

    std::string name = fname[1];

    std::string file_name = get_scripts_directory(username) + "/" + name + ".js";

    if(!file::exists(file_name))
    {
        std::cout << "Trying to create " << file_name << std::endl;

        file::write(file_name, "function(context, args)\n{\n\n}", file::mode::TEXT);
    }

    open_file(file_name);

    return "";
}

std::string on_open(std::string_view command, std::string_view username)
{
    std::string s_command(command.begin(), command.end());

    std::vector<std::string> fname = no_ss_split(s_command, " ");

    if(fname.size() < 2)
        return make_error_col("Format is #open scriptname");

    std::string name = fname[1];

    std::string file_name = get_scripts_directory(username) + "/" + name + ".js";

    if(!file::exists(file_name))
        return "No such file";

    open_file(file_name);

    return "";
}

void terminal2::on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send)
{
    /*if(text.starts_with("user "))
    {
        std::string_view without_prefix = text;
        without_prefix.remove_prefix(strlen("user "));

        if(without_prefix.size() > 0)
        {
            ctx.root_user = std::string(without_prefix.begin(), without_prefix.end());
        }
    }*/

    add_main_text(std::string(text.begin(), text.end()), auto_handle);
    add_main_text("");

    if(!is_local_command(text))
    {
        std::string up_data = default_up_handling(ctx.root_user, text, get_scripts_directory(ctx.root_user) + "/");

        nlohmann::json data;
        data["type"] = "generic_server_command";
        data["data"] = up_data;

        data["tag"] = tag;

        write_data dat;
        dat.id = -1;
        dat.data = data.dump();

        send.write_to_websocket(std::move(dat));
    }
    else
    {
        #ifndef __EMSCRIPTEN__
        file::mkdir("scripts");
        file::mkdir(get_scripts_directory(ctx.root_user).c_str());
        #endif // __EMSCRIPTEN__

        if(text == "#clear_autos" || text == "#autos_clear")
        {
            auto_handle.found_args.clear();
            auto_handle.is_valid.clear();
        }
        else if(text == "#shutdown")
        {
            ctx.should_shutdown = true;
        }
        else if(text.starts_with("#cls"))
        {
            clear_text();
        }
        else if(ctx.root_user == "")
        {
            add_main_text("Please log in with user <username>\n");
        }
        else if(text == "#")
        {
            add_main_text(get_scripts_list(ctx.root_user));
        }
        else if(text.starts_with("#edit "))
        {
            std::string res = on_edit(text, ctx.root_user);

            if(res != "")
                add_main_text(res);
        }
        else if(text.starts_with("#open "))
        {
            std::string res = on_open(text, ctx.root_user);

            if(res != "")
                add_main_text(res);
        }
        else if(text.starts_with("#dir"))
        {
            open_directory(get_scripts_directory(ctx.root_user));
        }
        else
        {
            add_main_text("Unhandled local command\n");
        }
    }

    command.push_command_to_history(text);
    command.clear_command();
}

void terminal2::extract_server_commands(context& ctx, nlohmann::json& in, auto_handler& auto_handle)
{
    if(in == "")
        return;

    ///this is absolute crap
    bool is_main_terminal = dynamic_cast<main_terminal2*>(this) != nullptr;

    std::string type = in["type"];

    if(type == "server_msg")
    {
        std::string data = in["data"];

        if(in.count("tag") > 0)
        {
            try
            {
                ///The client will only ever send integer tags to the server, but theoretically the tag type can be anything
                int found_tag = in["tag"];

                if(tag != found_tag)
                    return;
            }
            catch(...)
            {
                ///if there's a non integer tag, this message clearly isn't for us or even this client
                return;
            }
        }
        else
        {
            ///if there's no tag, return
            if(!is_main_terminal)
                return;
        }

        if(in.count("pad") == 0 || in["pad"] == 0)
        {
            data += "\n";
        }

        add_main_text(data, auto_handle);
    }
    else if(type == "chat_api" && is_main_terminal)
    {
        std::vector<std::string> chnls;
        std::vector<std::string> msgs;

        std::vector<nlohmann::json> tell_msgs = in["tells"];

        std::vector<std::string> notifs = in["notifs"];

        for(int i=0; i < (int)in["data"].size(); i++)
        {
            chnls.push_back(in["data"][i]["channel"]);
            msgs.push_back(in["data"][i]["text"]);
        }

        for(auto& i : notifs)
        {
            add_main_text(i + "\n");
        }

        ctx.user = in["user"];
        ctx.root_user = in["root_user"];

        for(int i=0; i < (int)chnls.size(); i++)
        {
            if(ctx.show_chat_in_main_window)
            {
                add_main_text(msgs[i] + "\n");
            }
        }

        for(int kk=0; kk < (int)tell_msgs.size(); kk++)
        {
            std::string text = tell_msgs[kk]["text"];

            add_main_text(text + "\n");
        }
    }
}

main_terminal2::main_terminal2()
{
    colour_like_terminal = true;
    command.command = "user ";
    command.cursor_pos_idx = command.command.size();
}

bool main_terminal2::create_window(context& ctx, vec2f content_size, vec2f in_window_size)
{
    ImGui::SetNextWindowContentSize({content_size.x(), content_size.y()});
    //ImGui::SetNextWindowSize(ImVec2(in_window_size.x(), in_window_size.y()), ImGuiCond_Appearing);

    int flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;

    vec2i real_window_size = ctx.backend->get_window_size();

    ImGui::SetNextWindowSize(ImVec2(real_window_size.x(), real_window_size.y()));
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    ImGui::SetNextWindowPos({window_pos.x(), window_pos.y()});
    ctx.backend->set_window_position({window_pos.x(), window_pos.y()});

    ImVec4 style_col = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive);

    ImGui::PushStyleColor(ImGuiCol_TitleBg, style_col);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, style_col);

    ImVec4 resize_col = ImGui::GetStyleColorVec4(ImGuiCol_ResizeGrip);
    ImU32 resize_colu32 = ImGui::ColorConvertFloat4ToU32(resize_col);

    bool rendering = ImGui::Begin(" NET_CODE_", &open, flags);

    if(ImGui::IsItemHovered() &&
       ImGui::IsMouseDragging(0) && !title_dragging && !resize_dragging)
    {
        if(!title_dragging)
        {
            title_dragging = true;
            title_start_pos = xy_to_vec(ImGui::GetMainViewport()->Pos);
        }
    }

    #ifndef __EMSCRIPTEN__
    if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        ctx.backend->set_is_maximised(!ctx.backend->is_maximised());
    }
    #endif // __EMSCRIPTEN__

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::MenuItem("> New Terminal"))
        {
            ctx.terminals.create_new_terminal();
        }

        ImGui::EndMenuBar();
    }

    if(title_dragging)
    {
        ImVec2 delta = ImGui::GetMouseDragDelta();

        ImVec2 real_pos;
        real_pos.x = delta.x + title_start_pos.x();
        real_pos.y = delta.y + title_start_pos.y();

        window_pos = {real_pos.x, real_pos.y};

        ctx.backend->set_window_position({window_pos.x(), window_pos.y()});
    }

    vec2f label_br = window_pos + window_size;
    vec2f label_tl = label_br - (vec2f){30, 30};

    bool hovering_label = ImGui::IsMouseHoveringRect({label_tl.x(), label_tl.y()}, {label_br.x(), label_br.y()}, true);

    if(hovering_label || resize_dragging)
        resize_colu32 = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ResizeGripActive));

    if(hovering_label)
    {
        if(ImGui::IsMouseDragging(0) && !title_dragging && !resize_dragging)
        {
            if(!resize_dragging)
            {
                resize_dragging = true;
                resize_start_pos = {window_size.x(), window_size.y()};
            }
        }
    }

    if(resize_dragging)
    {
        ImVec2 delta = ImGui::GetMouseDragDelta();

        int width = delta.x + resize_start_pos.x();
        int height = delta.y + resize_start_pos.y();

        if(width >= 50 && height >= 50)
            ctx.backend->resize({width, height});
    }

    if(!ImGui::IsMouseDown(0))
    {
        title_dragging = false;
        resize_dragging = false;
    }

    ImGui::GetWindowDrawList()->AddTriangleFilled({label_tl.x(), label_br.y()}, {label_br.x(), label_br.y()}, {label_br.x(), label_tl.y()}, resize_colu32);

    //printf("Pos %f %f\n", window_pos.x, window_pos.y);

    return rendering;
}

void main_terminal2::destroy_window()
{
    ImGui::End();

    ImGui::PopStyleColor(2);
}

void terminal_manager2::extract_server_commands(context& ctx, nlohmann::json& in, auto_handler& auto_handle)
{
    primary.extract_server_commands(ctx, in, auto_handle);

    for(auto& i : secondary)
    {
        i.extract_server_commands(ctx, in, auto_handle);
    }
}

void terminal_manager2::render(context& ctx, auto_handler& auto_handle)
{
    primary.render(ctx, auto_handle);

    for(auto& i : secondary)
    {
        i.render(ctx, auto_handle);
    }
}

void terminal_manager2::default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send)
{
    primary.default_controls(ctx, auto_handle, send);

    for(auto& i : secondary)
    {
        i.default_controls(ctx, auto_handle, send);
    }
}

void terminal_manager2::create_new_terminal()
{
    int max_id = 1;

    for(auto& i : secondary)
    {
        max_id = std::max(i.tag, max_id);
    }

    child_terminal& child = secondary.emplace_back();
    child.tag = max_id + 1;
}

child_terminal::child_terminal()
{
    colour_like_terminal = true;
}

void chat_manager::extract_server_commands(nlohmann::json& in)
{
    if(in == "")
        return;

    std::string type = in["type"];

    if(type == "chat_api")
    {
        std::vector<std::string> chnls;
        std::vector<std::string> msgs;

        std::vector<std::string> in_channels = in["channels"];

        std::vector<nlohmann::json> tell_msgs = in["tells"];

        for(int i=0; i < (int)in["data"].size(); i++)
        {
            chnls.push_back(in["data"][i]["channel"]);
            msgs.push_back(in["data"][i]["text"]);
        }

        set_open_chat_channels(in_channels);

        for(int i=0; i < (int)chnls.size(); i++)
        {
            chat_thread2& this_thread = chat_threads[chnls[i]];

            this_thread.add_main_text(msgs[i]);
        }
    }

    if(type == "chat_api_response")
    {
        std::string data = in["data"];

        for(const std::string& name : open_chat_channels)
        {
            chat_thread2& cthread = chat_threads[name];

            if(cthread.was_focused)
            {
                cthread.add_main_text(data);
            }
        }
    }
}

void chat_manager::set_open_chat_channels(const std::vector<std::string>& channels)
{
    std::set<std::string> logn(channels.begin(), channels.end());

    for(auto& [name, thread] : chat_threads)
    {
        if(logn.count(name) == 0)
        {
            thread.was_focused = false;
            thread.was_hovered = false;
            thread.was_rendered = false;
        }
    }

    open_chat_channels = channels;
}

void chat_manager::add_text(const std::string& channel, const std::vector<std::string>& text)
{
    for(const std::string& str : text)
    {
        chat_threads[channel].add_main_text(str);
    }
}

bool chat_thread2::create_window(context& ctx, vec2f content_size, vec2f create_window_size)
{
    create_window_size = {500, 300};

    ImGui::SetNextWindowContentSize({content_size.x(), content_size.y()});
    ImGui::SetNextWindowSize(ImVec2(create_window_size.x(), create_window_size.y()), ImGuiCond_Appearing);

    int flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;

    std::string unfriendly_name = friendly_name + "###" + friendly_name + "$";

    if(unseen_text)
        flags |= ImGuiWindowFlags_UnsavedDocument;

    return ImGui::Begin(unfriendly_name.c_str(), &open, flags);
}

void chat_thread2::on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send)
{
    if(text == "/join")
    {
        add_main_text("Syntax is /join channel password");
    }
    else if(text == "/leave")
    {
        add_main_text("Syntax is /leave channel");
    }
    else if(text == "/create")
    {
        add_main_text("Syntax is /create channel password");
    }
    else if(text.starts_with("/"))
    {
        int idx = 0;

        for(; idx < (int)text.size() && text[idx] != ' '; idx++);

        if(idx + 1 >= (int)text.size())
        {
            add_main_text("First argument must be a channel name, eg /join global");
        }
        else
        {
            idx++;

            std::string channel_name;

            for(; idx < (int)text.size() && text[idx] != ' '; idx++)
            {
                channel_name.push_back(text[idx]);
            }

            std::string channel_password;

            if(idx + 1 < (int)text.size())
            {
                idx++;

                ///password may include whitespace
                for(; idx < (int)text.size(); idx++)
                {
                    channel_password.push_back(text[idx]);
                }
            }

            std::string args = "{name:\"" + escape_str(channel_name) + "\"";

            if(channel_password != "")
            {
                args += ", password:\"" + escape_str(channel_password) + "\"}";
            }
            else
            {
                args += "}";
            }

            std::string final_command;

            if(text.starts_with("/join"))
            {
                final_command = "#channel.join(" + args + ")";
            }
            else if(text.starts_with("/leave"))
            {
                final_command = "#channel.leave(" + args + ")";
            }
            else if(text.starts_with("/create"))
            {
                final_command = "#channel.create(" + args + ")";
            }
            else
            {
                add_main_text("Not a valid command, try /join, /leave or /create");
            }

            if(final_command != "")
            {
                nlohmann::json data;
                data["type"] = "client_chat";
                data["respond"] = 1;
                data["data"] = final_command;

                write_data dat;
                dat.id = -1;
                dat.data = data.dump();

                send.write_to_websocket(std::move(dat));
            }
        }
    }
    else
    {
        std::string escaped_string = escape_str(text);

        nlohmann::json data;
        data["type"] = "client_chat";
        data["data"] = "#hs.msg.send({channel:\"" + friendly_name + "\", msg:\"" + escaped_string + "\"})";

        write_data dat;
        dat.id = -1;
        dat.data = data.dump();

        send.write_to_websocket(std::move(dat));
    }

    command.push_command_to_history(text);
    command.clear_command();
}

void chat_manager::default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send)
{
    for(const std::string& name : open_chat_channels)
    {
        chat_threads[name].default_controls(ctx, auto_handle, send);
    }
}

void chat_manager::render(context& ctx, auto_handler& auto_handle)
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
            ImGui::DockBuilderDockWindow(("###" + channel + "$").c_str(), dock_id);
        }

        ImGui::DockBuilderFinish(dock_id);

        file::write("ui_setup_once_v2", "1", file::mode::BINARY);
    }

    std::map<int, int> dock_ids;

    for(const std::string& channel_name : open_chat_channels)
    {
        chat_thread2& thread = chat_threads[channel_name];

        ImGui::SetNextWindowDockID(dock_id, ImGuiCond_FirstUseEver);

        thread.friendly_name = channel_name;

        thread.render(ctx, auto_handle);

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

/*void realtime_script_run2::render(connection_send_data& send)
{
    std::string str = std::to_string(i.first);

    std::string ext = "###" + str;

    std::string title_str = str;

    if(run.script_name != "")
    {
        title_str = run.script_name;
    }

    if(run.focused)
    {
        title_str += " (Active)";
    }
    else
    {
        title_str += "         "; ///for imgui docking grabbability
    }

    title_str += ext;

    if(run.was_open && !run.open)
    {
        run.was_open = false;

        was_closed_id = i.first;
    }

    if(!run.open)
        continue;

    if(run.set_size)
        ImGui::SetNextWindowSize(ImVec2(run.dim.x(), run.dim.y()), ImGuiCond_Always);

    run.set_size = false;

    bool should_render = ImGui::Begin((title_str + "###" + str).c_str(), &run.open, ImGuiWindowFlags_NoScrollbar);

    if(should_render)
    {
        run.current_tl_cursor_pos = {ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};

        run.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        run.hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        int cpos = -1;
        std::string cmd = " ";

        //run.cache.invalidate();

        if(run.is_square_font)
            ImGui::PushFont(fonts.get_square_font());

        render_ui_stack(to_write, run, run.stk, i.first, is_linear_colour);

        render_handle_imgui(run.scroll_hack, cmd, cpos, {run.parsed_data}, auto_handle, run.cache);

        ImVec2 window_size = ImGui::GetWindowSize();

        vec2i last_dim = run.current_dim;
        run.current_dim = {window_size.x, window_size.y};

        if(run.current_dim.x() != last_dim.x() || run.current_dim.y() != last_dim.y())
            run.should_send_new_size = true;

        if(run.is_square_font != run.was_square_font)
            run.should_send_new_size = true;

        run.was_square_font = run.is_square_font;

        if(run.should_send_new_size && run.last_resize.get_elapsed_time_s() >= 1)
        {
            vec2f br_absolute = run.current_pos + (vec2f){run.current_dim.x(), run.current_dim.y()};
            vec2f relative_dim = br_absolute - run.current_tl_cursor_pos;

            vec2f dim = relative_dim;

            vec2f cdim = xy_to_vec(ImGui::CalcTextSize("A"));

            nlohmann::json data;
            data["type"] = "send_script_info";
            data["id"] = i.first;
            data["width"] = (dim.x() / cdim.x()) - 5;
            data["height"] = dim.y() / cdim.y();

            write_data dat;
            dat.id = -1;
            dat.data = data.dump();

            to_write.write_to_websocket(std::move(dat));

            run.last_resize.restart();
            run.should_send_new_size = false;
        }

        if(run.is_square_font)
            ImGui::PopFont();

        auto my_pos = ImGui::GetWindowPos();

        run.current_pos = {my_pos.x, my_pos.y};
    }

    ImGui::End();
}*/

realtime_script_run2::realtime_script_run2()
{
    use_type_prompt = false;
}

bool realtime_script_run2::create_window(context& ctx, vec2f content_size, vec2f in_window_size)
{
    std::string str = std::to_string(server_id);

    std::string ext = "###" + str;

    std::string title_str = str;

    if(script_name != "")
    {
        title_str = script_name;
    }

    if(is_focused)
    {
        title_str += " (Active)";
    }
    else
    {
        title_str += "         "; ///for imgui docking grabbability
    }

    title_str += ext;

    ImGui::SetNextWindowContentSize({content_size.x(), content_size.y()});

    if(set_size)
        ImGui::SetNextWindowSize(ImVec2(dim.x(), dim.y()), ImGuiCond_Always);

    set_size = false;

    bool should_render = ImGui::Begin((title_str + "###" + str).c_str(), &open, ImGuiWindowFlags_NoScrollbar);

    is_focused = ImGui::IsWindowFocused();
    is_hovered = ImGui::IsWindowHovered();

    return should_render;
}

void realtime_script_run2::destroy_window()
{
    ImGui::End();
}

void realtime_script_run2::default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send)
{
    if(!is_focused)
        return;

    std::vector<std::string> input_string;

    std::vector<uint32_t> input_utf32;

    ImGuiIO& io = ImGui::GetIO();

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

            input_string.push_back(utf8);
        }
    }

    std::vector<std::string> on_pressed;
    std::vector<std::string> on_released;

    int key_count = 512;

    for(int i=0; i < key_count; i++)
    {
        if(!ImGui::IsKeyPressed(i))
            continue;

        if(auto val = ctx.backend->get_key_name(i); val != "")
            on_pressed.push_back(val);
    }

    for(int i=0; i < key_count; i++)
    {
        if(!ImGui::IsKeyReleased(i))
            continue;

        if(auto val = ctx.backend->get_key_name(i); val != "")
            on_released.push_back(val);
    }

    std::map<int, std::string> mouse_map;
    mouse_map[0] = "lmouse";
    mouse_map[1] = "rmouse";
    mouse_map[2] = "mmouse";
    mouse_map[3] = "x1mouse";
    mouse_map[4] = "x2mouse";

    for(int i=0; i < 5; i++)
    {
        if(ImGui::IsMouseClicked(i))
            on_pressed.push_back(mouse_map[i]);

        if(ImGui::IsMouseReleased(i))
            on_released.push_back(mouse_map[i]);
    }

    if(input_string.size() > 0 || on_pressed.size() > 0 || on_released.size() > 0)
    {
        nlohmann::json data;
        data["type"] = "send_keystrokes_to_script";
        data["id"] = server_id;
        data["input_keys"] = input_string;
        data["pressed_keys"] = on_pressed;
        data["released_keys"] = on_released;

        write_data dat;
        dat.id = -1;
        dat.data = data.dump();

        send.write_to_websocket(std::move(dat));
    }

    vec2f cursor_pos = {io.MousePos.x, io.MousePos.y};

    vec2f relative_mouse = cursor_pos - window_tl;

    vec2f mouse_char_pos = relative_mouse / get_char_size(font);

    unprocessed_scrollwheel += {io.MouseWheelH, io.MouseWheel};

    float mouse_ratelimit_ms = 33;

    if(is_hovered && mouse_send_ratelimit.get_elapsed_time_s() * 1000. >= mouse_ratelimit_ms)
    {
        nlohmann::json data;
        data["type"] = "update_mouse_to_script";
        data["id"] = server_id;
        data["mouse_x"] = mouse_char_pos.x();
        data["mouse_y"] = mouse_char_pos.y();
        data["mousewheel_x"] = unprocessed_scrollwheel.x();
        data["mousewheel_y"] = unprocessed_scrollwheel.y();

        write_data dat;
        dat.id = -1;
        dat.data = data.dump();

        send.write_to_websocket(std::move(dat));

        mouse_send_ratelimit.restart();
        unprocessed_scrollwheel = {0,0};
    }

    if(io.KeyCtrl && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_C]))
    {
        terminate(send);
    }
}

void realtime_script_run2::terminate(connection_send_data& send)
{
    open = false;

    nlohmann::json data;
    data["type"] = "client_terminate_scripts";
    data["id"] = server_id;

    write_data dat;
    dat.id = -1;
    dat.data = data.dump();

    send.write_to_websocket(std::move(dat));
}

void realtime_script_manager2::render(context& ctx, auto_handler& auto_handle)
{
    for(auto& i : windows)
    {
        realtime_script_run2& run = i.second;

        if(!run.open)
            continue;

        run.render(ctx, auto_handle);
    }
}

void realtime_script_manager2::extract_server_commands(font_selector& fonts, nlohmann::json& in)
{
    if(in == "")
        return;

    std::string type = in["type"];

    if(type == "command_realtime")
    {
        int id = in["id"];

        realtime_script_run2& run = windows[id];

        run.server_id = id;

        int width = 0;
        int height = 0;

        bool should_close = false;

        if(in.count("width") > 0)
            width = in["width"];
        if(in.count("height") > 0)
            height = in["height"];
        if(in.count("close") > 0)
            should_close = in["close"];
        if(in.count("square_font") > 0)
            run.is_square_font = (int)in["square_font"];

        if(!should_close && in.count("msg") > 0)
        {
            run.clear_text();
            run.add_main_text(in["msg"]);
        }

        if(should_close)
        {
            for(auto& i : windows)
            {
                int fid = i.first;
                realtime_script_run2& other_run = i.second;

                if(id == fid)
                {
                    other_run.open = false;
                }
            }
        }

        if(width != 0 && height != 0)
        {
            if(width < 5)
                width = 5;
            if(height < 5)
                height = 5;

            if(width > 300)
                width = 300;
            if(height > 300)
                height = 300;

            if(run.is_square_font)
                run.font = fonts.get_square_font();
            else
                run.font = nullptr;

            vec2f cdim = get_char_size(run.font);

            int rwidth = width * cdim.x();
            int rheight = height * cdim.y();

            run.dim.x() = rwidth;
            run.dim.y() = rheight;
        }

        if(in.count("name") > 0)
        {
            run.script_name = in["name"];
        }
    }
    else if(type == "command_realtime_ui")
    {
        int id = in["id"];

        realtime_script_run2& run = windows[id];

        run.server_id = id;

        std::map<std::string, ui_element> existing_elements;

        for(const ui_element& e : run.stk.elements)
        {
            if(e.element_id == "")
                continue;

            existing_elements[e.element_id] = e;
        }

        run.stk = ui_stack();

        std::vector<std::string> typelist = in["typeidx"];
        std::vector<int> typeargc = in["typeargc"];

        if(in.count("client_seq_ack") > 0)
            run.acked_sequence_id = in["client_seq_ack"];

        int num = in["types"].size();
        int current_argument_idx = 0;

        for(int i=0; i < num; i++)
        {
            int idx = in["types"][i];

            std::vector<nlohmann::json> arguments;

            std::string val = typelist.at(idx);
            int argument_count = typeargc.at(idx);

            for(int kk=0; kk < argument_count; kk++)
            {
                arguments.push_back(in["arguments"][kk + current_argument_idx]);
            }

            current_argument_idx += argument_count;

            //std::vector<nlohmann::json> arguments = (std::vector<nlohmann::json>)(in["arguments"][i]);

            std::string element_id = get_element_id(val, arguments);

            ui_element elem;

            if(auto it = existing_elements.find(element_id); it != existing_elements.end())
            {
                elem = it->second;
            }

            elem.element_id = element_id;

            elem.type = val;

            ///if the sequence id of us is > than the current acked id, it means we're client authoritative for a bit
            if((int)elem.arguments.size() != argument_count || run.acked_sequence_id >= elem.authoritative_until_sequence_id)
                elem.arguments = arguments;

            if((int)elem.arguments.size() != argument_count)
                throw std::runtime_error("Bad argument count somehow");

            run.stk.elements.push_back(elem);
        }
    }
}

void realtime_script_manager2::default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send)
{
    for(auto& i : windows)
    {
        realtime_script_run2& run = i.second;

        if(!run.open && run.open != run.was_open)
            run.terminate(send);

        run.was_open = run.open;

        if(!run.open)
            continue;

        run.default_controls(ctx, auto_handle, send);
    }
}

void test_render_strings()
{
    ImFont* font = ImGui::GetCurrentContext()->Font;

    std::string base = "hello there []\" asdf `Xcatepillar`\n`B`uncoloured\n`Dhithere\n``uncoloured`Dcoloured1`randomtext`Bcoloured2\n";

    auto_handler handle;

    std::vector<render_string> strs = create_render_strings(base, false, true);

    for(render_string& rstr : strs)
    {
        std::string_view view(base.begin() + rstr.start, base.begin() + rstr.start + rstr.length);

        std::cout << view << "|";
    }

    std::cout << "----\n";

    std::vector<screen_line> screen_lines = create_screen_lines(font, base, strs, 60);

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
