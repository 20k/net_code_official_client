#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include "copy_handler.hpp"
#include "font_cfg.hpp"
#include <GL/gl.h>
#include <GL/glext.h>
#include "local_commands.hpp"
#include <networking/networking.hpp>
#include <imgui/imgui_internal.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <toolkit/fs_helpers.hpp>
#include "auth_manager.hpp"
#include <toolkit/render_window.hpp>

void scrollbar_hack::do_hack(int approx_num, bool set_scrollbar, format_cache_2& cache, vec2f dim)
{
    int win_size = ImGui::GetWindowSize().y - 50;

    if(win_size < 0)
        return;

    ImGui::BeginChild("right_child", ImVec2(0, win_size), false, ImGuiWindowFlags_NoScrollWithMouse);

    vec2f cdim = xy_to_vec(ImGui::CalcTextSize("A"));
    cdim.y() = ImGui::GetTextLineHeightWithSpacing();

    if(approx_num < 0)
        approx_num = 1;

    ImGui::Dummy(ImVec2(cdim.x(), cdim.y() * (approx_num + 1)));

    int scroll_max_y = ImGui::GetScrollMaxY();

    if(set_scrollbar && scroll_max_y > 0)
    {
        ///scrolled is lines above 0
        float scrolled_frac = scrolled / approx_num;
        float ivscrolled = 1.f - scrolled_frac;

        ImGui::SetScrollY(ivscrolled * scroll_max_y);
    }

    float output_scroll_frac = 1;

    if(scroll_max_y > 0)
        output_scroll_frac = ImGui::GetScrollY() / scroll_max_y;

    if(ImGui::IsMouseDown(0) && ImGui::IsWindowFocused())
    {
        scrolling = true;
    }
    else
    {
        scrolling = false;
    }

    ImGui::EndChild();

    if(scrolling)
    {
        scrolled = (1.f - output_scroll_frac) * approx_num;
    }
}

void terminal_imgui::check_insert_user_command()
{
    if(one_time_user_insertion)
    {
        if(command.command.size() == 0)
        {
            command.command = "user ";
            command.cursor_pos_idx = command.command.size();
        }
    }

    one_time_user_insertion = false;
}

void terminal_imgui::clear_terminal()
{
    history.clear();
    raw_history.clear();
}

void chat_window::clear_chat()
{
    chat_threads.clear();
}

bool chat_window::any_cache_invalid()
{
    for(auto& i : side_buttons)
    {
        chat_thread& thread = chat_threads[i];

        if(!thread.cache.valid())
            return true;
    }

    return false;
}

void clear_everything(terminal_manager& term, chat_window& chat)
{
    term.main_terminal.clear_terminal();

    for(auto& i : term.sub_terminals)
    {
        i.second.clear_terminal();
    }

    chat.clear_chat();
}


void render_handle_imgui(scrollbar_hack& scroll_hack, std::string& command, int& cursor_pos_idx, const std::vector<interop_vec_t>& text_history, auto_handler& auto_handle, format_cache_2& cache, std::string command_padding = "")
{
    float overall_width = ImGui::GetWindowWidth();

    bool is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    ImGui::BeginChild("left_sub", ImVec2(overall_width - 40, 0.f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    if(ImGui::IsWindowHovered() && scroll_hack.scrolled_this_frame != 0)
    {
        scroll_hack.scrolled += scroll_hack.scrolled_this_frame;
        scroll_hack.scrolled_this_frame = 0.f;
    }

    ImGuiIO& io = ImGui::GetIO();

    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if(ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_PageDown]))
        {
            scroll_hack.scrolled -= ImGui::GetWindowHeight() / char_inf::cheight;

            if(scroll_hack.scrolled < 0)
                scroll_hack.scrolled = 0;
        }

        if(ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_PageUp]))
        {
            scroll_hack.scrolled += ImGui::GetWindowHeight() / char_inf::cheight;
        }
    }

    if(cache.was_focused != is_focused)
        cache.invalidate_last_line();

    cache.was_focused = is_focused;

    vec2f dim = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
    vec2f pos = {ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};

    if(dim.x() != cache.last_window_size.x())
        cache.invalidate();

    cache.last_window_size = dim;

    if(!cache.valid())
    {
        auto next_history = text_history;

        {
            std::string render_command = command;
            bool specials = true;

            if(render_command == "")
            {
                render_command = "`bType something here...`";
                specials = false;
            }

            auto icommand = string_to_interop(render_command, specials, auto_handle, false);
            auto icommand_pad = string_to_interop(command_padding, false, auto_handle, false);

            int cursor_offset = 0;

            auto_handle.handle_autocompletes(icommand, cursor_pos_idx, cursor_offset, command);

            interop_char curs;
            curs.col = {255, 255, 255};
            curs.c = '|';
            curs.is_cursor = true;

            int curs_cur = cursor_pos_idx + cursor_offset;

            if(is_focused)
            {
                if(curs_cur >= (int)icommand.size())
                    icommand.push_back(curs);
                else if(curs_cur >= 0 && curs_cur < (int)icommand.size())
                    icommand.insert(icommand.begin() + curs_cur, curs);
            }

            for(int i=0; i < (int)icommand_pad.size(); i++)
            {
                icommand.insert(icommand.begin() + i, icommand_pad[i]);
            }

            next_history.push_back(icommand);
        }

        cache.ensure_built(dim, next_history);
    }

    if(cache.must_rerender())
        cache.render_imgui(pos, dim, scroll_hack.scrolled);

    ImGui::EndChild();

    ImGui::SameLine(0.f, 0.f);

    vec2f cdim = xy_to_vec(ImGui::CalcTextSize("A"));

    //float flines = cache.get_lines() - (dim.y() / cdim.y());
    float flines = cache.get_lines();

    ///rough
    //ImGui::SetNextWindowContentSize({cache.last_content_size.x(), cache.last_content_size.y()});
    scroll_hack.do_hack(flines, true, cache, dim);
}

void terminal_imgui::render(terminal_manager& terminals, render_window& win, vec2f window_size, bool refocus, int extra_id)
{
    if(is_main_terminal)
    {
        copy_handler* handle = get_global_copy_handler();

        vec2f window_pos = xy_to_vec(ImGui::GetMainViewport()->Pos);

        ImGui::SetNextWindowSize(ImVec2(window_size.x(), window_size.y()));
        ImGui::SetNextWindowPos({window_pos.x(), window_pos.y()});
        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

        if(new_terminal)
        {
            win.resize({1400, 900});
            new_terminal = false;
        }

        if(refocus)
            ImGui::SetNextWindowFocus();

        ImVec4 style_col = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive);

        ImGui::PushStyleColor(ImGuiCol_TitleBg, style_col);
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, style_col);

        ImVec4 resize_col = ImGui::GetStyleColorVec4(ImGuiCol_ResizeGrip);
        ImU32 resize_colu32 = ImGui::ColorConvertFloat4ToU32(resize_col);

        ImGui::Begin(" NET_CODE_", &open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

        if(ImGui::IsItemHovered() &&
           ImGui::IsMouseDragging(0) && !title_dragging && !resize_dragging)
        {
            if(!title_dragging)
            {
                title_dragging = true;
                title_start_pos = ImGui::GetMainViewport()->Pos;
            }
        }

        ///TODO: FIXME
        #ifndef __EMSCRIPTEN__
        if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            win.backend->set_is_maximised(!win.backend->is_maximised());
        }
        #endif // __EMSCRIPTEN__

        if(ImGui::BeginMenuBar())
        {
            if(ImGui::MenuItem("> New Terminal"))
            {
                terminals.make_new_terminal();
            }

            ImGui::EndMenuBar();
        }

        vec2f window_br = window_pos + window_size;
        vec2f window_tl = window_br - (vec2f){30, 30};

        bool hovering_label = ImGui::IsMouseHoveringRect({window_tl.x(), window_tl.y()}, {window_br.x(), window_br.y()}, true);

        if(hovering_label || resize_dragging)
            resize_colu32 = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ResizeGripActive));

        if(hovering_label && !scroll_hack.scrolling)
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

        if(title_dragging)
        {
            ImVec2 delta = ImGui::GetMouseDragDelta();

            ImVec2 real_pos;
            real_pos.x = delta.x + title_start_pos.x;
            real_pos.y = delta.y + title_start_pos.y;

            win.backend->set_window_position({real_pos.x, real_pos.y});
        }

        if(resize_dragging)
        {
            ImVec2 delta = ImGui::GetMouseDragDelta();

            int width = delta.x + resize_start_pos.x;
            int height = delta.y + resize_start_pos.y;

            if(width >= 50 && height >= 50)
                win.resize({width, height});
        }

        if(!ImGui::IsMouseDown(0))
        {
            title_dragging = false;
            resize_dragging = false;
        }

        focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        if(refocus)
            ImGui::SetNextWindowFocus();

        render_handle_imgui(scroll_hack, command.command, command.cursor_pos_idx, history, terminals.auto_handle, cache, colour_string(current_user) + "> ");

        ImGui::GetWindowDrawList()->AddTriangleFilled({window_tl.x(), window_br.y()}, {window_br.x(), window_br.y()}, {window_br.x(), window_tl.y()}, resize_colu32);

        ImGui::End();

        ImGui::PopStyleColor(2);
    }
    else
    {
        if(new_terminal)
        {
            ImGui::SetNextWindowSize({500, 300}, ImGuiCond_Always);
            new_terminal = false;
        }

        std::string extra = "TERM" + std::to_string(friendly_id) + "##" + std::to_string(extra_id);

        ImGui::Begin(extra.c_str(), &open, ImGuiWindowFlags_NoScrollbar);

        focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        render_handle_imgui(scroll_hack, command.command, command.cursor_pos_idx, history, terminals.auto_handle, cache, colour_string(current_user) + "> ");

        ImGui::End();
    }
}

///all values from the server are sanitised in some way unless explicitly noted otherwise
///that is: randomised salted hashes in the strings to prevent collisions
///strings have a capped length
///doubles are not nan, not that json supports that anyway
///all values are clamped to sensible ranges so they can be piped directly into imgui
///colours are clamped to [0, 1]
void render_ui_stack(connection& conn, realtime_script_run& run, ui_stack& stk, int id, bool is_linear_colour)
{
    ImGui::BeginGroup();

    int group_unbalanced_stack = 0;
    int push_colour_stack = 0;
    int item_width_stack = 0;

    for(ui_element& e : stk.elements)
    {
        if(e.type == "text")
        {
            if(e.arguments.size() < 1)
                continue;

            std::string val = e.arguments[0];

            ImGui::TextUnformatted(val.c_str(), val.c_str() + val.size());
        }

        if(e.type == "textcolored")
        {
            ///r, g, b, a, text
            if(e.arguments.size() < 5)
                continue;

            float r = e.arguments[0];
            float g = e.arguments[1];
            float b = e.arguments[2];
            float a = e.arguments[3];

            if(is_linear_colour)
            {
                r = srgb_to_lin_approx((vec1f)r).x();
                g = srgb_to_lin_approx((vec1f)g).x();
                b = srgb_to_lin_approx((vec1f)b).x();
                a = srgb_to_lin_approx((vec1f)a).x();
            }

            std::string val = e.arguments[4];

            ImGui::TextColored(ImVec4(r, g, b, a), "%s", val.c_str());
        }

        if(e.type == "textdisabled")
        {
            if(e.arguments.size() < 1)
                continue;

            std::string val = e.arguments[0];

            ImGui::TextDisabled("%s", val.c_str());
        }

        if(e.type == "bullettext")
        {
            if(e.arguments.size() < 1)
                continue;

            std::string val = e.arguments[0];

            ImGui::BulletText("%s", val.c_str());
        }

        if(e.type == "button" || e.type == "smallbutton" || e.type == "invisiblebutton" || e.type == "arrowbutton")
        {
            if(e.arguments.size() < 1)
                continue;

            std::string val = e.arguments[0];

            if(e.type == "button")
            {
                if(e.arguments.size() < 3)
                    continue;

                double w = e.arguments[1];
                double h = e.arguments[2];

                ImGui::Button(val.c_str(), ImVec2(w, h));
            }

            if(e.type == "smallbutton")
                ImGui::SmallButton(val.c_str());

            if(e.type == "invisiblebutton")
            {
                if(e.arguments.size() < 3)
                    continue;

                double w = e.arguments[1];
                double h = e.arguments[2];

                ImGui::InvisibleButton(val.c_str(), ImVec2(w, h));
            }

            if(e.type == "arrowbutton")
            {
                if(e.arguments.size() < 2)
                    continue;

                int dir = e.arguments[1];

                ImGui::ArrowButton(val.c_str(), dir);
            }

            std::vector<std::string> states;

            bool is_hovered = ImGui::IsItemHovered();
            bool was_hovered = e.was_hovered;

            bool is_clicked = ImGui::IsItemClicked();

            if(is_hovered)
                states.push_back("hovered");

            if(is_clicked)
                states.push_back("clicked");

            if(is_hovered != was_hovered || is_clicked)
            {
                nlohmann::json j;
                j["type"] = "client_ui_element";
                j["id"] = id;
                j["ui_id"] = e.element_id;
                j["state"] = states;

                conn.write(j.dump());
            }

            e.was_hovered = is_hovered;
        }

        if(e.type == "bullet")
        {
            ImGui::Bullet();
        }

        if(e.type == "pushstylecolor")
        {
            if(e.arguments.size() < 5)
                continue;

            ///IDX IS NOT SANITISED
            int idx = e.arguments[0];
            double r = e.arguments[1];
            double g = e.arguments[2];
            double b = e.arguments[3];
            double a = e.arguments[4];

            if(is_linear_colour)
            {
                r = srgb_to_lin_approx((vec1f)r).x();
                g = srgb_to_lin_approx((vec1f)g).x();
                b = srgb_to_lin_approx((vec1f)b).x();
                a = srgb_to_lin_approx((vec1f)a).x();
            }

            ///PANIC AND BREAK THINGS
            if(idx >= 0 && idx < ImGuiCol_COUNT)
            {
                push_colour_stack++;

                ImGui::PushStyleColor(idx, ImVec4(r, g, b, a));
            }
        }

        if(e.type == "popstylecolor")
        {
            if(e.arguments.size() < 1)
                continue;

            int idx = e.arguments[0];

            while(push_colour_stack > 0 && idx > 0)
            {
                push_colour_stack--;
                idx--;
                ImGui::PopStyleColor(1);
            }
        }

        if(e.type == "pushitemwidth")
        {
            if(e.arguments.size() < 1)
                continue;

            double width = e.arguments[0];

            item_width_stack++;
            ImGui::PushItemWidth(width);
        }

        if(e.type == "popitemwidth")
        {
            if(item_width_stack > 0)
            {
                item_width_stack--;
                ImGui::PopItemWidth();
            }
        }

        if(e.type == "setnextitemwidth")
        {
            if(e.arguments.size() < 1)
                continue;

            double width = e.arguments[0];

            ImGui::SetNextItemWidth(width);
        }

        if(e.type == "separator")
        {
            ImGui::Separator();
        }

        if(e.type == "sameline")
        {
            if(e.arguments.size() < 2)
                continue;

            double offset_from_start = e.arguments[0];
            double spacing = e.arguments[1];

            ImGui::SameLine(offset_from_start, spacing);
        }

        if(e.type == "newline")
        {
            ImGui::NewLine();
        }

        if(e.type == "spacing")
        {
            ImGui::Spacing();
        }

        if(e.type == "dummy")
        {
            if(e.arguments.size() < 2)
                continue;

            double w = e.arguments[0];
            double h = e.arguments[1];

            ImGui::Dummy(ImVec2(w, h));
        }

        if(e.type == "indent")
        {
            if(e.arguments.size() < 1)
                continue;

            double amount = e.arguments[0];

            ImGui::Indent(amount);
        }

        if(e.type == "unindent")
        {
            if(e.arguments.size() < 1)
                continue;

            double amount = e.arguments[0];

            ImGui::Unindent(amount);
        }

        if(e.type == "begingroup")
        {
            group_unbalanced_stack++;

            ImGui::BeginGroup();
        }

        if(e.type == "endgroup")
        {
            group_unbalanced_stack--;

            ImGui::EndGroup();
        }
    }

    for(int i=0; i < group_unbalanced_stack; i++)
    {
        ImGui::EndGroup();
    }

    while(push_colour_stack > 0)
    {
        push_colour_stack--;
        ImGui::PopStyleColor(1);
    }

    while(item_width_stack > 0)
    {
        item_width_stack--;
        ImGui::PopItemWidth();
    }

    ImGui::EndGroup();
}

void realtime_script_manager::render_realtime_windows(connection& conn, int& was_closed_id, font_selector& fonts, auto_handler& auto_handle, bool is_linear_colour)
{
    was_closed_id = -1;

    copy_handler* handle = get_global_copy_handler();

    for(auto& i : windows)
    {
        realtime_script_run& run = i.second;

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

            render_ui_stack(conn, run, run.stk, i.first, is_linear_colour);

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

                conn.write(data.dump());

                run.last_resize.restart();
                run.should_send_new_size = false;
            }

            if(run.is_square_font)
                ImGui::PopFont();

            auto my_pos = ImGui::GetWindowPos();

            run.current_pos = {my_pos.x, my_pos.y};
        }

        ImGui::End();
    }
}

int realtime_script_manager::get_id_of_focused_realtime_window()
{
    for(auto& i : windows)
    {
        if(i.second.focused && i.second.open)
            return i.first;
    }

    return -1;
}

void invalidate_everything(terminal_manager& term, chat_window& chat)
{
    term.main_terminal.cache.invalidate();

    for(auto& i : term.sub_terminals)
    {
        i.second.cache.invalidate();
    }

    for(auto& i : chat.chat_threads)
    {
        i.second.cache.invalidate();
    }
}

void last_line_invalidate_everything(terminal_manager& term, chat_window& chat)
{
    term.main_terminal.cache.invalidate_last_line();

    for(auto& i : term.sub_terminals)
    {
        i.second.cache.invalidate_last_line();
    }

    for(auto& i : chat.chat_threads)
    {
        i.second.cache.invalidate_last_line();
    }
}

void terminal_imgui::bump_command_to_history(auto_handler& auto_handle)
{
    std::string ccommand = command.command;

    interop_vec_t interop;
    std::string raw;

    if(ccommand != "" && current_user != "")
    {
        raw = colour_string(current_user) + "> ";
        interop = string_to_interop_no_autos(raw, false);
    }

    auto interop_command = string_to_interop(ccommand, true, auto_handle);

    for(auto& i : interop_command)
    {
        interop.push_back(i);
    }

    raw_history.push_back(raw + ccommand);
    history.push_back(interop);
    command.clear_command();

    limit_size(raw_history, MAX_TEXT_HISTORY);
    limit_size(history, MAX_TEXT_HISTORY);
}

void fix_tabs(std::string& str)
{
    int tab_width = 4;

    for(int i=0; i < (int)str.size(); i++)
    {
        if(str[i] == '\t')
        {
            str[i] = ' ';

            for(int k=0; k < tab_width - 1; k++)
            {
                str.insert(str.begin() + i, ' ');
            }
        }
    }
}

void terminal_imgui::add_text(const std::string& str, auto_handler& auto_handle)
{
    raw_history.push_back(str);
    history.push_back(string_to_interop(str, false, auto_handle));

    limit_size(raw_history, MAX_TEXT_HISTORY);
    limit_size(history, MAX_TEXT_HISTORY);
    consider_resetting_scrollbar = true;

    cache.invalidate();
}

void terminal_imgui::extend_text(const std::string& str, auto_handler& auto_handle)
{
    if(raw_history.size() == 0)
        return add_text(str, auto_handle);

    assert(history.size() > 0);

    std::string old = raw_history.back();

    raw_history.pop_back();
    history.pop_back();

    add_text(old + str, auto_handle);
}

terminal_manager::terminal_manager()
{
    main_terminal.is_main_terminal = true;

    auto_handle.use_autocomplete = true;
    auto_handle.use_autocolour = true;
}

void terminal_manager::render(render_window& win, vec2f window_size, bool refocus)
{
    //for(auto& i : sub_terminals)

    for(auto it = sub_terminals.begin(); it != sub_terminals.end();)
    {
        it->second.render(*this, win, window_size, refocus, it->first);

        if(!it->second.open)
        {
            it = sub_terminals.erase(it);
        }
        else
        {
            it++;
        }
    }

    main_terminal.render(*this, win, window_size, refocus, -1);
}

bool terminal_manager::all_cache_valid()
{
    for(auto& i : sub_terminals)
    {
        if(!i.second.cache.valid())
            return false;
    }

    if(!main_terminal.cache.valid())
        return false;

    return true;
}

void terminal_manager::invalidate_visual_cache()
{
    for(auto& i : sub_terminals)
    {
        i.second.cache.invalidate_visual_cache();
    }

    main_terminal.cache.invalidate_visual_cache();
}

terminal_imgui* terminal_manager::get_focused_terminal()
{
    for(auto& i : sub_terminals)
    {
        if(i.second.focused)
        {
            return &i.second;
        }
    }

    return &main_terminal;
}

int terminal_manager::get_focused_terminal_id()
{
    for(auto& i : sub_terminals)
    {
        if(i.second.focused)
        {
            return i.first;
        }
    }

    return -1;
}

void terminal_manager::make_new_terminal()
{
    int max_id = 0;

    for(auto& i : sub_terminals)
    {
        max_id = std::max(i.second.friendly_id, max_id);
    }

    terminal_imgui& term = sub_terminals[gid++];
    term.friendly_id = max_id+1;
}

void process_text_from_server(terminal_manager& terminals, auth_manager& auth_manage, std::string& in_user, const nlohmann::json& in, chat_window& chat_win, font_selector& fonts, realtime_script_manager& realtime_scripts)
{
    if(in == "")
        return;

    terminal_imgui* term = &terminals.main_terminal;

    std::string str;

    /*std::string str = in;

    fix_tabs(str);*/

    std::string type = in["type"];

    bool push = false;

    if(type == "server_msg")
    {
        str = in["data"];

        fix_tabs(str);

        if(in.count("tag") > 0)
        {
            printf("Tag?\n");

            try
            {
                int tag = in["tag"];

                if(auto it = terminals.sub_terminals.find(tag); it != terminals.sub_terminals.end())
                {
                    term = &it->second;
                }

            }
            catch(...){}
        }

        if(in.count("pad") == 0 || in["pad"] == 0)
        {
            str += "\n";
        }

        if(in.count("authenticated") > 0 && in["authenticated"] == 1)
        {
            auth_manage.am_authenticated = true;

            printf("YAY!\n");
        }

        push = true;
    }
    else if(type == "command_realtime")
    {
        int id = in["id"];

        realtime_script_run& run = realtime_scripts.windows[id];

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
        {
            if(run.is_square_font != (int)in["square_font"])
                run.cache.invalidate();

            run.is_square_font = (int)in["square_font"];
        }

        run.last_message.restart();

        if(!should_close && in.count("msg") > 0)
        {
            run.parsed_data = string_to_interop_no_autos(in["msg"], false);
            run.cache.invalidate();
        }

        if(should_close)
        {
            for(auto& i : realtime_scripts.windows)
            {
                int fid = i.first;
                realtime_script_run& run = i.second;

                if(id == fid)
                {
                    run.open = false;
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

            run.set_size = true;

            vec2f cdim = xy_to_vec(ImGui::CalcTextSize("A"));

            if(run.is_square_font)
            {
                ImGui::PushFont(fonts.get_square_font());

                cdim = xy_to_vec(ImGui::CalcTextSize("A"));

                ImGui::PopFont();
            }

            int rwidth = width * cdim.x();
            int rheight = height * cdim.y();

            if(rwidth != run.dim.x())
                run.cache.invalidate();

            run.dim.x() = rwidth;
            run.dim.y() = rheight;
        }

        if(in.count("name") > 0)
        {
            run.script_name = in["name"];
        }
    }
    else if(in["type"] == "command_realtime_ui")
    {
        int id = in["id"];

        realtime_script_run& run = realtime_scripts.windows[id];

        nlohmann::json data = in["msg"];

        std::map<std::string, ui_element> existing_elements;

        for(const ui_element& e : run.stk.elements)
        {
            if(e.element_id == "")
                continue;

            existing_elements[e.element_id] = e;
        }

        run.stk = ui_stack();

        for(auto& e : data)
        {
            ui_element elem;

            std::string element_id = e["element_id"];

            if(auto it = existing_elements.find(element_id); it != existing_elements.end())
            {
                elem = it->second;
            }

            elem.type = e["type"];
            elem.arguments = (std::vector<nlohmann::json>)e["arguments"];
            elem.element_id = element_id;

            run.stk.elements.push_back(elem);
        }
    }
    else if(in["type"] == "chat_api")
    {
        std::vector<std::string> chnls;
        std::vector<std::string> msgs;

        std::vector<std::string> in_channels = in["channels"];

        std::vector<nlohmann::json> tell_msgs = in["tells"];

        std::vector<std::string> notifs = in["notifs"];

        for(int i=0; i < (int)in["data"].size(); i++)
        {
            chnls.push_back(in["data"][i]["channel"]);
            msgs.push_back(in["data"][i]["text"]);
        }

        for(auto& i : notifs)
        {
            fix_tabs(i);

            term->raw_history.push_back(i + "\n");
            term->history.push_back(string_to_interop_no_autos(i + "\n", false));
        }

        if(notifs.size() > 0)
            term->cache.invalidate();

        if(tell_msgs.size() > 0)
            term->cache.invalidate();

        std::string next_user = in["user"];

        if(next_user != term->current_user)
        {
            term->cache.invalidate();
            term->current_user = next_user;
        }

        std::string root_user = in["root_user"];

        in_user = root_user;

        chat_win.set_side_channels(in_channels);

        for(int i=0; i < (int)chnls.size(); i++)
        {
            fix_tabs(msgs[i]);

            if(chat_win.show_chat_in_main_window)
            {
                term->raw_history.push_back(msgs[i]);
                term->history.push_back(string_to_interop(msgs[i] + "\n", false, chat_win.auto_handle));

                term->cache.invalidate();
            }

            chat_win.chat_threads[chnls[i]].raw_history.push_back(msgs[i]);
            chat_win.chat_threads[chnls[i]].history.push_back(string_to_interop(msgs[i], false, chat_win.auto_handle));
            chat_win.chat_threads[chnls[i]].dirty = true;
            chat_win.chat_threads[chnls[i]].cache.invalidate();

            limit_size(chat_win.chat_threads[chnls[i]].raw_history, MAX_TEXT_HISTORY);
            limit_size(chat_win.chat_threads[chnls[i]].history, MAX_TEXT_HISTORY);
        }

        for(int kk=0; kk < (int)tell_msgs.size(); kk++)
        {
            nlohmann::json js = tell_msgs[kk];

            std::string text = js["text"];

            fix_tabs(text);

            term->raw_history.push_back(text + "\n");
            term->history.push_back(string_to_interop_no_autos(text + "\n", false));
        }

        limit_size(term->raw_history, MAX_TEXT_HISTORY);
        limit_size(term->history, MAX_TEXT_HISTORY);
    }
    else if(in["type"] == "script_args")
    {
        std::string scriptname = in["script"];

        std::cout << scriptname << std::endl;

        if(scriptname.size() > 0)
        {
            std::vector<autocomplete_args> auto_args;

            for(int i=0; i < (int)in["keys"].size(); i++)
            {
                std::string key = in["keys"][i];
                std::string val = in["vals"][i];

                auto_args.push_back({key, val});
            }

            terminals.auto_handle.found_args[scriptname] = auto_args;
            terminals.auto_handle.is_valid[scriptname] = true;
        }
    }
    else if(in["type"] == "script_args_invalid")
    {
        std::string scriptname = in["script"];

        std::cout << "inv " << scriptname << std::endl;

        if(scriptname.size() > 0)
        {
            terminals.auto_handle.is_valid[scriptname] = false;
        }
    }
    else if(in["type"] == "script_args_ratelimit")
    {
        std::string name = in["script"];

        std::cout << "rl name " << name << std::endl;

        if(name.size() > 0)
        {
            terminals.auto_handle.found_unprocessed_autocompletes.push_back(name);
        }
    }
    else if(in["type"] == "server_ping")
    {
        ///do nothing
    }
    else if(in["type"] == "script_down")
    {
        std::string name = in["name"];
        std::string data = in["data"];

        auto post_split = no_ss_split(name, ".");

        if(post_split.size() >= 2)
        {
            std::string save_name = get_scripts_directory(post_split[0]) + "\\" + post_split[1] + ".down.js";

            file::write(save_name, data, file::mode::TEXT);

            #ifdef __EMSCRIPTEN__
            std::string webdlname = name + ".down.js";
            file::download(webdlname, data);
            #endif // __EMSCRIPTEN__

            str = make_success_col("Downloaded and saved script to " + get_scripts_directory(post_split[0]) + "\\" + post_split[1] + ".down.js") + "\n";
            push = true;
        }
    }
    else if(in["type"] == "chat_api_response")
    {
        std::string data = in["data"];

        fix_tabs(data);

        chat_win.add_text_to_focused(data);
    }
    else if(in["type"] == "auth")
    {
        std::string key = in["data"];
        std::string key_file = "hex_key.key";

        if(!file::exists(key_file))
        {
            file::write(key_file, key, file::mode::BINARY);

            term->add_text(make_success_col("Success! Try user lowercase_name to get started, and then #scripts.core()"), terminals.auto_handle);
        }
        else
        {
            term->add_text(make_error_col("Did not overwrite existing key file, you are already registered"), terminals.auto_handle);
        }
    }
    else
    {
        push = true;
    }

    if(push)
    {
        fix_tabs(str);

        term->raw_history.push_back(str);
        term->history.push_back(string_to_interop(str, false, terminals.auto_handle));

        limit_size(term->raw_history, MAX_TEXT_HISTORY);
        limit_size(term->history, MAX_TEXT_HISTORY);
        term->consider_resetting_scrollbar = true;

        term->cache.invalidate();
    }
}

void chat_window::render(bool refocus)
{
    copy_handler* handle = get_global_copy_handler();

    if(refocus && side_buttons.size() > 0)
        ImGui::SetNextWindowFocus();

    static bool once = file::exists("ui_setup_once");
    static ImGuiID dock_id = -1;

    if(!once || ImGui::IsKeyPressed(GLFW_KEY_F2))
    {
        dock_id = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
        ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;
        ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
        ImGui::DockBuilderSetNodePos(dock_id, ImVec2(viewport_pos.x + viewport_size.x - 600, viewport_pos.y + 100));
        ImGui::DockBuilderSetNodeSize(dock_id, ImVec2(500, 300));

        for(int i=0; i < (int)side_buttons.size(); i++)
        {
            ImGui::DockBuilderDockWindow(("###" + side_buttons[i]).c_str(), dock_id);
        }

        ImGui::DockBuilderFinish(dock_id);

        file::write("ui_setup_once", "1", file::mode::BINARY);
    }

    std::map<int, int> dock_ids;

    bool any_focused = false;
    bool any_hovered = false;

    for(int i=0; i < (int)side_buttons.size(); i++)
    {
        std::string full_str = side_buttons[i];
        chat_thread& thread = chat_threads[side_buttons[i]];
        thread.was_rendered = false;

        if(thread.dirty && !thread.was_focused)
            full_str += "*";
        else
            full_str += " ";

        full_str += "###" + side_buttons[i];

        ImGui::SetNextWindowDockID(dock_id, ImGuiCond_FirstUseEver);

        bool should_render = ImGui::Begin(full_str.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

        if(should_render)
        {
            if(dock_id == (ImGuiID)-1)
            {
                dock_ids[ImGui::GetWindowDockID()]++;
            }

            bool me_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

            thread.was_focused = me_focused;
            thread.name = side_buttons[i];

            if(me_focused)
            {
                thread.dirty = false;
                any_focused = true;

                for(auto& text : unprocessed_input)
                {
                    thread.raw_history.push_back(text);
                    thread.history.push_back(string_to_interop(text, false, auto_handle, false));
                }

                limit_size(thread.raw_history, MAX_TEXT_HISTORY);
                limit_size(thread.history, MAX_TEXT_HISTORY);

                unprocessed_input.clear();
            }

            if(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
            {
                any_hovered = true;
                thread.was_hovered = true;
            }

            render_handle_imgui(thread.scroll_hack, thread.command.command, thread.command.cursor_pos_idx, thread.history, auto_handle, thread.cache);
            thread.was_rendered = true;
        }

        ImGui::End();
    }

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

    focused = any_focused;
    hovered = any_hovered;

    once = true;

    unprocessed_input.clear();
}

void chat_window::set_side_channels(const std::vector<std::string>& sides)
{
    side_buttons = sides;
}

std::optional<editable_string*> chat_window::get_focused_editable()
{
    for(auto& i : chat_threads)
    {
        if(i.second.was_focused)
            return &i.second.command;
    }

    return std::nullopt;
}

std::optional<chat_thread*> chat_window::get_focused_chat_thread()
{
    for(auto& i : chat_threads)
    {
        if(i.second.was_focused)
            return &i.second;
    }

    return std::nullopt;
}

void chat_window::add_text_to_focused(const std::string& str)
{
    unprocessed_input.push_back(str);
}
