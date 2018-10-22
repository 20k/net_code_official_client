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

void terminal_imgui::do_serialise(serialise& s, bool ser)
{
    if(ser == false)
    {
        auto_handle.found_unprocessed_autocompletes.clear();
        auto_handle.found_args.clear();
        auto_handle.is_valid.clear();
    }

    s.handle_serialise(history, ser);
    s.handle_serialise(chat_threads, ser);
    s.handle_serialise(command, ser);
    s.handle_serialise_no_clear(auto_handle, ser);

    //std::cout << "loaded hist " << text_history.size() << std::endl;
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

enum render_instruction
{
    imgui_text,
    newline,
};

struct render_command
{
    render_instruction type = imgui_text;
    std::string str;
    vec3f col;
    vec2f absolute_pos;
};

void render_copy_aware(font_render_context& font_select, vec3f col, const std::string& str, vec2f start_pos, vec2f end_pos, vec2f render_pos)
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

        ImDrawList* imlist = ImGui::GetWindowDrawList();
        imlist->AddText(ImVec2(c_pos.x(), c_pos.y()), IM_COL32((int)ccol.x(), (int)ccol.y(), (int)ccol.z(), 255), cstr.c_str());

        c_pos.x() += cstr.size() * char_inf::cwidth;

        if(i != (int)cols.size() - 1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

void render_copy_blind(font_render_context& font_select, vec3f col, const std::string& str, vec2f render_pos)
{
    ImGui::SetCursorScreenPos(ImVec2(render_pos.x(), render_pos.y()));

    //ImGuiX::TextColoredUnformatted(ImVec4(col.x()/255.f, col.y()/255.f, col.z()/255.f, 1.f), str.c_str());

    ImDrawList* imlist = ImGui::GetWindowDrawList();
    imlist->AddText(ImVec2(render_pos.x(), render_pos.y()), IM_COL32((int)col.x(), (int)col.y(), (int)col.z(), 255), str.c_str());
}

void imgui_render_str(font_render_context& font_select, const std::vector<formatted_char>& text, std::vector<std::vector<formatted_char>>& formatted_text, float window_width)
{
    copy_handler* handle = get_global_copy_handler();

    std::vector<render_command> commands;

    render_command current;

    bool restart = true;

    for(const formatted_char& fchar : text)
    {
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
                current.type = imgui_text;
                commands.push_back(current);
                current = render_command();
            }

            if(fchar.ioc.c == '\n' || fchar.render_pos.y() != current.absolute_pos.y())
            {
                if(current.str.size() > 0)
                {
                    current.type = imgui_text;
                    commands.push_back(current);
                    current = render_command();
                }

                render_command next;
                next.type = newline;
                commands.push_back(next);
                //continue;
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
        render_command next;
        next.type = newline;
        commands.push_back(next);
    }

    formatted_text.emplace_back();

    std::vector<formatted_char>& chars = formatted_text.back();

    for(int kk=0; kk < (int)commands.size(); kk++)
    {
        render_command& next = commands[kk];

        if(next.type == newline)
        {
            //ImGui::Text("\n");
            continue;
        }

        vec2f pos = next.absolute_pos;
        std::string str = next.str;
        vec3f col = next.col;

        //auto spos = ImGui::GetCursorScreenPos();

        auto spos = pos;

        ///need to predict here if the text is hilighted or not
        ///then if it is, replace spaces with "-" and colour blue
        float width = ImGui::CalcTextSize(str.c_str(), nullptr, false, window_width).x;

        if(handle->held && ImGui::IsWindowFocused())
            render_copy_aware(font_select, col, str, spos, (vec2f){spos.x(), spos.y()} + (vec2f){width, 0.f}, pos);
        else
            render_copy_blind(font_select, col, str, pos);

        //std::cout << "pos " << pos << std::endl;

        float x_start = spos.x();
        float x_end = spos.x() + width;

        float y_coord = spos.y();

        for(int ccount = 0; ccount < (int)str.size(); ccount++)
        {
            float ffrac = (float)ccount / (float)str.size();

            formatted_char chr;
            chr.ioc.c = str[ccount];

            chr.render_pos.y() = y_coord;
            chr.render_pos.x() = ffrac * x_end + (1.f - ffrac) * x_start;

            chars.push_back(chr);
        }

        /*if(kk == (int)commands.size()-1)
            ImGui::NewLine();*/

        if(kk != (int)commands.size()-1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

bool render_handle_imgui(font_render_context& font_select, scrollbar_hack& scroll_hack, std::string& command, int& cursor_pos_idx, const std::vector<interop_vec_t>& text_history, auto_handler& auto_handle, format_cache& cache, frameable& frame, float extra_shrink = 0, std::string command_padding = "")
{
    float overall_width = ImGui::GetWindowWidth();

    ImGui::BeginChild("left_sub", ImVec2(overall_width - 40 - extra_shrink, 0.f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    auto cpos = ImGui::GetWindowPos();

    if(cpos.x != cache.last_pos.x() || cpos.y != cache.last_pos.y())
        cache.invalidate();

    cache.last_pos = {cpos.x, cpos.y};

    if(ImGui::IsWindowHovered() && scroll_hack.scrolled_this_frame != 0)
    {
        scroll_hack.scrolled += scroll_hack.scrolled_this_frame;
        scroll_hack.scrolled_this_frame = 0.f;
    }

    if(!cache.valid())
    {
        auto wrap_dim = ImGui::GetWindowSize();
        auto start = ImGui::GetWindowPos();

        wrap_dim.x += start.x;
        wrap_dim.y += start.y;

        vec2f current = {start.x, start.y};

        int vertical_rows = ceil((float)wrap_dim.y / char_inf::cheight);

        frame.render_height = vertical_rows;

        /*int min_start = (int)text_history.size() - vertical_rows;

        min_start = min_start - scroll_hack.scrolled;

        if(min_start < 0)
            min_start = 0;*/

        //std::vector<std::vector<formatted_char>> formatted;

        auto all_interop = text_history;

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

    cache.out.clear();

    //ImDrawList* imlist = ImGui::GetWindowDrawList();

    //imlist->AddDrawCmd();

    for(auto& i : cache.cache)
    {
        imgui_render_str(font_select, i, cache.out, ImGui::GetWindowWidth());
    }

    //std::cout << "dsize " << imlist->CmdBuffer[imlist->CmdBuffer.size() - 1].ElemCount << std::endl;

    //imlist->AddDrawCmd();

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

void terminal_imgui::render(font_render_context& font_select, sf::RenderWindow& win, bool refocus)
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

    bool child_focused = render_handle_imgui(font_select, scroll_hack, command.command, command.cursor_pos_idx, history, auto_handle, cache, *this, 0.f, colour_string(current_user) + "> ");

    ImGui::End();

    if(focused && child_focused)
        handle->process_formatted(cache.out);
}

void terminal_imgui::render_realtime_windows(font_render_context& font_select, c_shared_data data, int& was_closed_id)
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

        bool child_focused = render_handle_imgui(font_select, run.scroll_hack, cmd, cpos, {run.parsed_data}, auto_handle, run.cache, *this);

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

            sa_do_send_script_info(data, i.first, (dim.x() / char_inf::cwidth) - 5, dim.y() / char_inf::cheight);

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

#define MAX_TEXT_HISTORY 200

void terminal_imgui::bump_command_to_history()
{
    std::string ccommand = command.command;

    interop_vec_t interop;

    if(ccommand != "" && current_user != "")
    {
        interop = string_to_interop_no_autos(colour_string(current_user) + "> ", false);
    }

    auto interop_command = string_to_interop(ccommand, true, auto_handle);

    for(auto& i : interop_command)
    {
        interop.push_back(i);
    }

    history.push_back(interop);
    command.clear_command();

    limit_size(history, MAX_TEXT_HISTORY);
    de_newline(history);
}

void terminal_imgui::add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command)
{
    if(in == "")
        return;

    std::string str = in;

    server_command_info command_info = sa_server_response_to_info(make_view(in));

    bool push = false;

    if(server_command)
    {
        if(command_info.type == server_command_command)
        {
            str = c_str_consume(sa_command_to_human_readable(command_info)) + "\n";

            invalidate();

            push = true;
        }
        else if(command_info.type == server_command_command_realtime)
        {
            realtime_info info = sa_command_realtime_to_info(command_info);

            realtime_script_windows[info.id].last_message.restart();

            if(!info.should_close && info.msg.num > 0)
                realtime_script_windows[info.id].parsed_data = string_to_interop_no_autos(c_str_sized_to_cpp(info.msg), false);

            if(info.should_close)
            {
                for(auto& i : realtime_script_windows)
                {
                    int id = i.first;
                    realtime_script_run& run = i.second;

                    if(id == info.id)
                    {
                        run.open = false;
                    }
                }
            }

            if(info.width != 0 && info.height != 0)
            {
                if(info.width < 5)
                    info.width = 5;
                if(info.height < 5)
                    info.height = 5;

                if(info.width > 300)
                    info.width = 300;
                if(info.height > 300)
                    info.height = 300;

                realtime_script_windows[info.id].set_size = true;

                int rwidth = info.width * char_inf::cwidth;
                int rheight = info.height * char_inf::cheight;

                realtime_script_windows[info.id].dim.x() = rwidth;
                realtime_script_windows[info.id].dim.y() = rheight;
            }

            if(info.name.num > 0)
            {
                realtime_script_windows[info.id].script_name = c_str_sized_to_cpp(info.name);
            }

            sa_destroy_realtime_info(info);
        }
        else if(command_info.type == server_command_chat_api)
        {
            std::vector<std::string> chnls;
            std::vector<std::string> msgs;

            std::vector<std::string> in_channels;

            std::vector<std::string> tell_msgs;

            chat_api_info chat_info = sa_chat_api_to_info(command_info);

            for(int i=0; i < chat_info.num_msgs; i++)
            {
                chnls.push_back(c_str_sized_to_cpp(chat_info.msgs[i].channel));
                msgs.push_back(c_str_sized_to_cpp(chat_info.msgs[i].msg));
            }

            for(int i=0; i < chat_info.num_in_channels; i++)
            {
                in_channels.push_back(c_str_sized_to_cpp(chat_info.in_channels[i].channel));
            }

            for(int i=0; i < chat_info.num_tells; i++)
            {
                tell_msgs.push_back(c_str_sized_to_cpp(chat_info.tells[i].msg));
            }

            for(int i=0; i < chat_info.num_notifs; i++)
            {
                history.push_back(string_to_interop_no_autos(c_str_sized_to_cpp(chat_info.notifs[i].msg), false));
            }

            if(chat_info.num_tells > 0 || chat_info.num_notifs > 0 || chat_info.num_msgs > 0)
                invalidate();

            std::string next_user = c_str_sized_to_cpp(chat_info.current_user);

            if(next_user != current_user)
            {
                cache.invalidate();
                current_user = next_user;
            }

            sa_destroy_chat_api_info(chat_info);

            chat_win.set_side_channels(in_channels);

            for(int i=0; i < (int)chnls.size(); i++)
            {
                if(chat_win.show_chat_in_main_window)
                    history.push_back(string_to_interop(msgs[i], false, chat_win.auto_handle));

                chat_threads[chnls[i]].history.push_back(string_to_interop(msgs[i], false, chat_win.auto_handle));
                chat_threads[chnls[i]].dirty = true;
            }

            for(auto& i : tell_msgs)
            {
                history.push_back(string_to_interop_no_autos(i, false));
            }

            limit_size(history, MAX_TEXT_HISTORY);
            de_newline(history);
        }
        else if(command_info.type == server_command_server_scriptargs)
        {
            std::cout << str << std::endl;

            script_argument_list args = sa_server_scriptargs_to_list(command_info);

            if(args.scriptname.str != nullptr && args.scriptname.num > 0)
            {
                std::vector<autocomplete_args> auto_args;

                for(int i=0; i < args.num; i++)
                {
                    std::string key = c_str_sized_to_cpp(args.args[i].key);
                    std::string val = c_str_sized_to_cpp(args.args[i].val);

                    auto_args.push_back({key, val});
                }

                std::string scriptname = c_str_sized_to_cpp(args.scriptname);

                auto_handle.found_args[scriptname] = auto_args;
                auto_handle.is_valid[scriptname] = true;
            }

            sa_destroy_script_argument_list(args);
        }
        else if(command_info.type == server_command_server_scriptargs_invalid)
        {
            std::cout << "inv " << str << std::endl;

            std::string name = c_str_consume(sa_server_scriptargs_invalid_to_script_name(command_info));

            if(name.size() > 0)
            {
                auto_handle.is_valid[name] = false;
            }
        }
        else if(command_info.type == server_command_server_scriptargs_ratelimit)
        {
            std::string name = c_str_consume(sa_server_scriptargs_ratelimit_to_script_name(command_info));

            std::cout << "rl name " << name << std::endl;

            if(name.size() > 0)
            {
                auto_handle.found_unprocessed_autocompletes.insert(name);
            }
        }
        else if(command_info.type == server_command_command_tagged)
        {
            command_tagged_info tag_info = sa_command_tagged_to_info(command_info);

            str = c_str_sized_to_cpp(tag_info.val);

            std::string tag = c_str_sized_to_cpp(tag_info.tag);

            tag_manager& tag_manage = get_global_tag_manager();

            tag_manage.add_tagged(tag, str);

            push = true;
        }
        else if(command_info.type == server_command_command_ping)
        {
            ///do nothing
        }
        else if(starts_with(str, "command_auth"))
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
        history.push_back(string_to_interop(str, false, auto_handle));

        limit_size(history, MAX_TEXT_HISTORY);
        consider_resetting_scrollbar = true;

        de_newline(history);

        invalidate();
    }

    sa_destroy_server_command_info(command_info);
}

void chat_thread::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(history, ser);
}

void chat_window::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(render_start, ser);
    s.handle_serialise(side_buttons, ser);
    s.handle_serialise(selected, ser);
    s.handle_serialise(command, ser);
    s.handle_serialise(show_chat_in_main_window, ser);
    //s.handle_serialise(focus_once, ser);
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

void chat_window::render(font_render_context& font_select, sf::RenderWindow& win, std::map<std::string, chat_thread>& threads, bool refocus)
{
    copy_handler* handle = get_global_copy_handler();

    chat_thread& thread = threads[selected];
    thread.dirty = false;

    if(refocus)
        ImGui::SetNextWindowFocus();

    ImGui::SetNextWindowSize(ImVec2(dim.x(), dim.y()), ImGuiCond_FirstUseEver);
    //ImGui::SetNextWindowPos(ImVec2(win.getSize().x - dim.x() * 1.08f, ImGuiCond_FirstUseEver));

    std::string chat_str = selected + "###chat_window";

    ImGui::Begin(chat_str.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ResizeFromAnySide | ImGuiWindowFlags_MenuBar);

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("Settings"))
        {
            ImGui::Checkbox("Show in main window", &show_chat_in_main_window);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    ImGui::BeginChild("left_selector", ImVec2(80, 0));

    int max_width = 60;

    for(auto& i : side_buttons)
    {
        std::string name = i;

        if(threads[i].dirty && !show_chat_in_main_window)
            name += "*";

        int width = ImGui::CalcTextSize(name.c_str()).x;

        max_width = std::max(width, max_width);
    }

    max_width += ImGui::GetStyle().ItemInnerSpacing.x * 2;

    for(auto& i : side_buttons)
    {
        std::string name = i;

        if(threads[i].dirty && !show_chat_in_main_window)
            name += "*";

        if(ImGui::Button(name.c_str(), ImVec2(max_width, 0)))
        {
            selected = i;
        }
    }

    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    if(refocus)
        ImGui::SetNextWindowFocus();

    bool child_focused = render_handle_imgui(font_select, scroll_hack, command.command, command.cursor_pos_idx, thread.history, auto_handle, thread.cache, *this, 80);

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
