#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include <libncclient/nc_string_interop.hpp>
#include <libncclient/c_server_api.h>
#include "copy_handler.hpp"
#include "tag_manager.hpp"
#include "font_cfg.hpp"
#include <GL/gl.h>
#include <gl/glext.h>
#include "window_context.hpp"
#include "local_commands.hpp"
#include "imguix.hpp"
#include <networking/networking.hpp>

namespace ImGuiX
{
    void TextColoredUnformatted(const ImVec4& col, const char* text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }
}

void scrollbar_hack::do_hack(int approx_num, bool set_scrollbar)
{
    ImGui::BeginChild("right_child", ImVec2(0,0), false, ImGuiWindowFlags_NoScrollWithMouse);

    for(int i=0; i < approx_num; i++)
    {
        ImGui::Text("\n");
    }

    if(set_scrollbar)
    {
        ///scrolled is lines above 0
        float scrolled_frac = scrolled / approx_num;
        float ivscrolled = 1.f - scrolled_frac;

        ImGui::SetScrollY(ivscrolled * ImGui::GetScrollMaxY());
    }

    output_scroll_frac = ImGui::GetScrollY() / ImGui::GetScrollMaxY();

    if(ImGui::IsMouseDown(0) && ImGui::IsWindowFocused())
    {
        scrolling = true;
    }
    else
    {
        scrolling = false;
    }

    ImGui::EndChild();
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

void terminal_imgui::clear_chat()
{
    chat_threads.clear();
}

void terminal_imgui::clear_text()
{
    clear_terminal();
    clear_chat();
}

terminal_imgui::terminal_imgui()
{
    auto_handle.use_autocomplete = true;
    auto_handle.use_autocolour = true;
}

struct render_command
{
    std::string str;
    vec3f col;
    vec2f absolute_pos;
    bool copyable = true;
};

void render_copy_aware(vec3f col, const std::string& str, vec2f start_pos, vec2f end_pos, vec2f render_pos)
{
    std::vector<std::pair<std::string, vec3f>> cols;

    std::pair<std::string, vec3f> cur_str;

    if(str.size() == 0)
        return;

    copy_handler* handle = get_global_copy_handler();
    bool is_inside = handle->char_is_within_select_box(start_pos);

    for(int i=0; i < (int)str.size(); i++)
    {
        float ffrac = (float)i / (float)str.size();

        vec2f pos = mix(start_pos, end_pos, ffrac);

        bool currently_inside = handle->char_is_within_select_box(pos);

        if(currently_inside != is_inside)
        {
            cols.push_back(cur_str);
            cur_str = decltype(cur_str)();
            is_inside = currently_inside;
        }

        cur_str.first.push_back(str[i]);

        if(currently_inside)
        {
            cur_str.second = {80, 80, 255};

            if(cur_str.first.back() == ' ')
                cur_str.first.back() = '-';
        }
        else
            cur_str.second = col;
    }

    if(cur_str.first.size() > 0)
    {
        cols.push_back(cur_str);
    }

    vec2f c_pos = render_pos;

    for(int i=0; i < (int)cols.size(); i++)
    {
        vec3f ccol = cols[i].second;
        const std::string& cstr = cols[i].first;

        ImGui::SetCursorScreenPos(ImVec2(c_pos.x(), c_pos.y()));

        //ImGuiX::TextColoredUnformatted(ImVec4(ccol.x()/255.f, ccol.y()/255.f, ccol.z()/255.f, 1.f), cstr.c_str());

        if(ImGui::IsStyleLinearColor())
            ccol = srgb_to_lin(ccol/255.f) * 255.f;

        ImDrawList* imlist = ImGui::GetWindowDrawList();
        imlist->AddText(ImVec2(c_pos.x(), c_pos.y()), IM_COL32((int)ccol.x(), (int)ccol.y(), (int)ccol.z(), 255), cstr.c_str());

        //c_pos.x() += cstr.size() * char_inf::cwidth;

        c_pos.x() += ImGui::CalcTextSize(cstr.c_str(), nullptr).x;

        if(i != (int)cols.size() - 1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

void render_copy_blind(vec3f col, const std::string& str, vec2f render_pos)
{
    ImGui::SetCursorScreenPos(ImVec2(render_pos.x(), render_pos.y()));

    //ImGuiX::TextColoredUnformatted(ImVec4(col.x()/255.f, col.y()/255.f, col.z()/255.f, 1.f), str.c_str());

    if(ImGui::IsStyleLinearColor())
        col = srgb_to_lin(col/255.f) * 255.f;

    ImDrawList* imlist = ImGui::GetWindowDrawList();
    imlist->AddText(ImVec2(render_pos.x(), render_pos.y()), IM_COL32((int)col.x(), (int)col.y(), (int)col.z(), 255), str.c_str());
}

void imgui_render_str(const std::vector<formatted_char>& text, float window_width)
{
    copy_handler* handle = get_global_copy_handler();

    std::vector<render_command> commands;

    render_command current;

    bool restart = true;

    for(const formatted_char& fchar : text)
    {
        if(fchar.ioc.is_cursor)
        {
            render_command next;
            next.col = {255, 255, 255};
            next.str = "|";
            next.absolute_pos = fchar.render_pos;
            next.copyable = fchar.copyable;

            commands.push_back(next);
            continue;
        }

        if(restart)
        {
            current.absolute_pos = fchar.render_pos;
            restart = false;
        }

        if(fchar.ioc.col != current.col || fchar.ioc.c == '\n' || fchar.render_pos.y() != current.absolute_pos.y())
        {
            restart = true;

            if(current.str.size() > 0 && fchar.ioc.c != '\n')
            {
                commands.push_back(current);
                current = render_command();
            }

            if(fchar.ioc.c == '\n' || fchar.render_pos.y() != current.absolute_pos.y())
            {
                if(current.str.size() > 0)
                {
                    commands.push_back(current);
                    current = render_command();
                }
            }
        }

        if(restart)
        {
            current.absolute_pos = fchar.render_pos;
            restart = false;
        }

        current.col = fchar.ioc.col;
        current.str += fchar.ioc.c;
    }

    if(current.str.size() > 0)
    {
        commands.push_back(current);
    }

    if(commands.size() == 0)
    {
        return;
    }

    for(int kk=0; kk < (int)commands.size(); kk++)
    {
        render_command& next = commands[kk];

        vec2f pos = next.absolute_pos;
        std::string str = next.str;
        vec3f col = next.col;

        ///need to predict here if the text is hilighted or not
        ///then if it is, replace spaces with "-" and colour blue
        float width = ImGui::CalcTextSize(str.c_str(), nullptr, false, window_width).x;
        //float width = str.size() * char_inf::cwidth;

        if(handle->held && ImGui::IsWindowFocused() && next.copyable)
            render_copy_aware(col, str, pos, (vec2f){pos.x(), pos.y()} + (vec2f){width, 0.f}, pos);
        else
            render_copy_blind(col, str, pos);

        if(kk != (int)commands.size()-1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

bool render_handle_imgui(scrollbar_hack& scroll_hack, std::string& command, int& cursor_pos_idx, const std::vector<interop_vec_t>& text_history, auto_handler& auto_handle, format_cache& cache, frameable& frame, float extra_shrink = 0, std::string command_padding = "")
{
    float overall_width = ImGui::GetWindowWidth();

    ImGui::BeginChild("left_sub", ImVec2(overall_width - 40 - extra_shrink, 0.f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    auto cpos = ImGui::GetWindowPos();

    if(cpos.x != cache.last_pos.x() || cpos.y != cache.last_pos.y())
        cache.invalidate();

    cache.last_pos = {cpos.x, cpos.y};

    if(ImGui::IsWindowHovered() && scroll_hack.scrolled_this_frame != 0)
    {
        cache.cached_line_offset += scroll_hack.scrolled_this_frame;

        scroll_hack.scrolled += scroll_hack.scrolled_this_frame;
        scroll_hack.scrolled_this_frame = 0.f;

        //cache.invalidate();
    }

    auto current_window_size = ImGui::GetWindowSize();

    if(current_window_size.x != cache.cached_window_size.x() || current_window_size.y != cache.cached_window_size.y())
        cache.invalidate();

    if(!cache.valid())
    {
        auto wrap_dim = ImGui::GetWindowSize();
        auto start = ImGui::GetWindowPos();

        wrap_dim.x += start.x;
        wrap_dim.y += start.y;

        vec2f current = {start.x, start.y};

        int vertical_rows = ceil((float)wrap_dim.y / char_inf::cheight);

        frame.render_height = vertical_rows;

        std::vector<interop_vec_t> all_interop;

        if(!cache.valid_cache)
            all_interop = text_history;

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

        if(ImGui::IsWindowFocused())
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

        all_interop.push_back(icommand);

        cache.ensure_built(current, {start.x, start.y}, {wrap_dim.x, wrap_dim.y}, all_interop, scroll_hack, vertical_rows);
    }

    auto ccache = cache.get_render_cache();

    for(auto& i : ccache)
    {
        imgui_render_str(i, ImGui::GetWindowWidth());
    }

    cache.out.clear();
    cache.out = ccache;

    bool text_area_focused = ImGui::IsWindowFocused();

    ImGui::EndChild();

    ImGui::SameLine(0.f, 0.f);

    ///rough
    scroll_hack.do_hack(cache.get_lines(), true);

    if(scroll_hack.scrolling)
    {
        scroll_hack.scrolled = (1.f - scroll_hack.output_scroll_frac) * (cache.get_lines() + 1.f);

        cache.invalidate();
    }

    return text_area_focused;
}

void terminal_imgui::render(sf::RenderWindow& win, bool refocus)
{
    copy_handler* handle = get_global_copy_handler();

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(win.getSize().x, win.getSize().y));

    if(refocus)
        ImGui::SetNextWindowFocus();

    ImGui::Begin("asdf1", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    if(refocus)
        ImGui::SetNextWindowFocus();

    bool child_focused = render_handle_imgui(scroll_hack, command.command, command.cursor_pos_idx, history, auto_handle, cache, *this, 0.f, colour_string(current_user) + "> ");

    ImGui::End();

    if(focused && child_focused)
        handle->process_formatted(cache.out);
}

void terminal_imgui::render_realtime_windows(connection& conn, int& was_closed_id)
{
    was_closed_id = -1;

    copy_handler* handle = get_global_copy_handler();

    for(auto& i : realtime_script_windows)
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

        ImGui::Begin((title_str + "###" + str).c_str(), &run.open);

        run.current_tl_cursor_pos = {ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};

        run.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        run.hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        int cpos = -1;
        std::string cmd = " ";

        run.cache.invalidate();

        bool child_focused = render_handle_imgui(run.scroll_hack, cmd, cpos, {run.parsed_data}, auto_handle, run.cache, *this);

        if(run.focused && child_focused)
            handle->process_formatted(run.cache.out);

        ImVec2 window_size = ImGui::GetWindowSize();

        vec2i last_dim = run.current_dim;
        run.current_dim = {window_size.x, window_size.y};

        if(run.current_dim.x() != last_dim.x() || run.current_dim.y() != last_dim.y())
        {
            run.should_send_new_size = true;
        }

        if(run.should_send_new_size && run.last_resize.getElapsedTime().asSeconds() >= 1)
        {
            vec2f br_absolute = run.current_pos + (vec2f){run.current_dim.x(), run.current_dim.y()};
            vec2f relative_dim = br_absolute - run.current_tl_cursor_pos;

            vec2f dim = relative_dim;

            nlohmann::json data;
            data["type"] = "send_script_info";
            data["id"] = i.first;
            data["width"] = (dim.x() / char_inf::cwidth) - 5;
            data["height"] = dim.y() / char_inf::cheight;

            conn.write(data.dump());

            run.last_resize.restart();
            run.should_send_new_size = false;
        }

        auto my_pos = ImGui::GetWindowPos();

        run.current_pos = {my_pos.x, my_pos.y};

        ImGui::End();
    }
}

int terminal_imgui::get_id_of_focused_realtime_window()
{
    for(auto& i : realtime_script_windows)
    {
        if(i.second.focused && i.second.open)
            return i.first;
    }

    return -1;
}

void terminal_imgui::invalidate()
{
    cache.invalidate();

    for(auto& i : chat_threads)
    {
        i.second.cache.invalidate();
    }
}

void terminal_imgui::last_line_invalidate()
{
    cache.last_line_invalidate();

    for(auto& i : chat_threads)
    {
        i.second.cache.last_line_invalidate();
    }
}

#define MAX_TEXT_HISTORY 200

void terminal_imgui::bump_command_to_history()
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
    de_newline(history);
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

void terminal_imgui::add_text(const std::string& str)
{
    raw_history.push_back(str);
    history.push_back(string_to_interop(str, false, auto_handle));

    limit_size(raw_history, MAX_TEXT_HISTORY);
    limit_size(history, MAX_TEXT_HISTORY);
    consider_resetting_scrollbar = true;

    de_newline(history);

    invalidate();
}

void terminal_imgui::add_text_from_server(std::string& in_user, const nlohmann::json& in, chat_window& chat_win, bool server_command)
{
    if(in == "")
        return;

    std::string str;

    /*std::string str = in;

    fix_tabs(str);*/

    std::string type = in["type"];

    bool push = false;

    if(server_command)
    {
        if(type == "server_msg")
        {
            str = in["data"];

            fix_tabs(str);

            if(in.count("tag") > 0)
            {
                std::string tag = in["tag"];

                tag_manager& tag_manage = get_global_tag_manager();

                tag_manage.add_tagged(tag, str);
            }

            if(in.count("pad") == 0 || in["pad"] == 0)
            {
                str += "\n";
            }

            invalidate();

            push = true;
        }
        else if(type == "command_realtime")
        {
            int id = in["id"];

            int width = 0;
            int height = 0;

            bool should_close = false;

            if(in.count("width") > 0)
                width = in["width"];
            if(in.count("height") > 0)
                height = in["height"];
            if(in.count("close") > 0)
                should_close = in["close"];

            realtime_script_windows[id].last_message.restart();

            if(!should_close && in.count("msg") > 0)
                realtime_script_windows[id].parsed_data = string_to_interop_no_autos(in["msg"], false);

            if(should_close)
            {
                for(auto& i : realtime_script_windows)
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

                realtime_script_windows[id].set_size = true;

                int rwidth = width * char_inf::cwidth;
                int rheight = height * char_inf::cheight;

                realtime_script_windows[id].dim.x() = rwidth;
                realtime_script_windows[id].dim.y() = rheight;
            }

            if(in.count("name") > 0)
            {
                realtime_script_windows[id].script_name = in["name"];
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

                raw_history.push_back(i + "\n");
                history.push_back(string_to_interop_no_autos(i + "\n", false));
            }

            if(tell_msgs.size() > 0 || notifs.size() > 0 || msgs.size() > 0)
                invalidate();

            std::string next_user = in["user"];

            if(next_user != current_user)
            {
                cache.invalidate();
                current_user = next_user;
            }

            std::string root_user = in["root_user"];

            in_user = root_user;

            chat_win.set_side_channels(in_channels);

            for(int i=0; i < (int)chnls.size(); i++)
            {
                fix_tabs(msgs[i]);

                if(chat_win.show_chat_in_main_window)
                {
                    raw_history.push_back(msgs[i]);
                    history.push_back(string_to_interop(msgs[i] + "\n", false, chat_win.auto_handle));
                }

                chat_threads[chnls[i]].raw_history.push_back(msgs[i]);
                chat_threads[chnls[i]].history.push_back(string_to_interop(msgs[i], false, chat_win.auto_handle));
                chat_threads[chnls[i]].dirty = true;
            }

            for(int kk=0; kk < (int)tell_msgs.size(); kk++)
            {
                nlohmann::json js = tell_msgs[kk];

                std::string text = js["text"];

                fix_tabs(text);

                raw_history.push_back(text + "\n");
                history.push_back(string_to_interop_no_autos(text + "\n", false));
            }

            limit_size(raw_history, MAX_TEXT_HISTORY);
            limit_size(history, MAX_TEXT_HISTORY);
            de_newline(history);
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

                auto_handle.found_args[scriptname] = auto_args;
                auto_handle.is_valid[scriptname] = true;
            }
        }
        else if(in["type"] == "script_args_invalid")
        {
            std::string scriptname = in["script"];

            std::cout << "inv " << scriptname << std::endl;

            if(scriptname.size() > 0)
            {
                auto_handle.is_valid[scriptname] = false;
            }
        }
        else if(in["type"] == "script_args_ratelimit")
        {
            std::string name = in["script"];

            std::cout << "rl name " << name << std::endl;

            if(name.size() > 0)
            {
                auto_handle.found_unprocessed_autocompletes.push_back(name);
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

            std::string save_name = get_scripts_directory() + "/" + name + ".down.js";

            write_all_bin(save_name, data);

            str = make_success_col("Downloaded and saved script to " + name + ".down.js") + "\n";
            push = true;
        }
        else if(in["type"] == "chat_api_response")
        {
            std::string data = in["data"];

            fix_tabs(data);

            add_text_to_current_chat_thread(chat_win, data);

            invalidate();
        }
        else if(in["type"] == "auth")
        {
            ///do nothing
        }
        else
        {
            push = true;
        }
    }
    else
    {
        push = true;

        invalidate();
    }

    if(push)
    {
        fix_tabs(str);

        raw_history.push_back(str);
        history.push_back(string_to_interop(str, false, auto_handle));

        limit_size(raw_history, MAX_TEXT_HISTORY);
        limit_size(history, MAX_TEXT_HISTORY);
        consider_resetting_scrollbar = true;

        de_newline(history);

        invalidate();
    }
}

void terminal_imgui::add_text_to_current_chat_thread(chat_window& chat_win, const std::string& text)
{
    chat_thread& thr = chat_threads[chat_win.selected];

    thr.raw_history.push_back(text);
    thr.history.push_back(string_to_interop(text, false, auto_handle, false));
    thr.dirty = true;
}

sf::Color chat_window::get_frame_col()
{
    return sf::Color(frame_col.x()*255.f, frame_col.y()*255.f, frame_col.z()*255.f, 255);
}

sf::Color chat_window::get_highlight_col()
{
    return sf::Color(70, 70, 70, 255);
}

void chat_window::tick()
{

}

void chat_window::render(sf::RenderWindow& win, std::map<std::string, chat_thread>& threads, bool refocus)
{
    copy_handler* handle = get_global_copy_handler();

    chat_thread& thread = threads[selected];
    thread.dirty = false;

    if(refocus)
        ImGui::SetNextWindowFocus();

    ImGui::SetNextWindowSize(ImVec2(dim.x(), dim.y()), ImGuiCond_FirstUseEver);
    //ImGui::SetNextWindowPos(ImVec2(win.getSize().x - dim.x() * 1.08f, ImGuiCond_FirstUseEver));

    std::string chat_str = selected;

    ImGui::Begin((chat_str + "###chat_window").c_str());

    if(ImGui::BeginMenuBar())
    {
        ImGuiX::Text(chat_str);

        if(ImGui::BeginMenu("Settings"))
        {
            ImGui::Checkbox("Show in main window", &show_chat_in_main_window);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::BeginChild("chat_child", ImVec2(80, 0));

    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    int max_len = 0;

    for(int i=0; i < (int)side_buttons.size(); i++)
    {
        max_len = std::max(side_buttons[i].size(), (size_t)max_len);
    }

    for(int i=0; i < (int)side_buttons.size(); i++)
    {
        std::string text = side_buttons[i];

        if(threads[side_buttons[i]].dirty && !show_chat_in_main_window)
        {
            text += "*";
        }

        for(int kk=(int)text.size(); kk < max_len; kk++)
        {
            text += " ";
        }

        if(ImGui::Button(text.c_str()))
        {
            selected = side_buttons[i];
        }
    }

    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    if(refocus)
        ImGui::SetNextWindowFocus();

    bool child_focused = render_handle_imgui(scroll_hack, command.command, command.cursor_pos_idx, thread.history, auto_handle, thread.cache, *this, 80);

    ImGui::End();

    if(focused && child_focused)
        handle->process_formatted(thread.cache.out);
}

void chat_window::set_side_channels(const std::vector<std::string>& sides)
{
    side_buttons.clear();

    bool any = false;

    for(auto& i : sides)
    {
        side_buttons.push_back(i);

        if(i == selected)
        {
            any = true;
        }
    }

    if(!any && side_buttons.size() > 0)
    {
        selected = side_buttons.front();
    }

    if(!any && side_buttons.size() == 0)
    {
        selected = "No channels";
    }
}
