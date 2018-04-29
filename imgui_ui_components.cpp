#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include <libncclient/nc_string_interop.hpp>
#include <libncclient/c_server_api.h>
#include "ui_components.hpp"
#include "copy_handler.hpp"

void scrollbar_hack::do_hack(sf::RenderWindow& win, int approx_num, bool set_scrollbar)
{
    /*float base_y = win.getSize().y;

    float my_y = base_y - scrolled * char_inf::cheight;

    float max_y = base_y - approx_num * char_inf::cheight;

    if(max_y < -char_inf::cheight)
    {
        std::cout << "hack at " << max_y << std::endl;

        //ImGui::SetCursorScreenPos(ImVec2(0.f, max_y));
        ImGui::SetCursorPos(ImVec2(0, max_y));
        ImGui::Text("Invis");
    }*/

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

    s.handle_serialise(text_history, ser);
    s.handle_serialise(chat_threads, ser);
    s.handle_serialise(command, ser);
    s.handle_serialise_no_clear(auto_handle, ser);

    //std::cout << "loaded hist " << text_history.size() << std::endl;
}

void terminal_imgui::clear_terminal()
{
    text_history.clear();
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

void render_copy_aware(sf::RenderWindow& win, vec3f col, const std::string& str, vec2f start_pos, vec2f end_pos, vec2f render_pos)
{
    int cutoff_point = -1;

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

        ImGui::TextColored(ImVec4(ccol.x()/255.f, ccol.y()/255.f, ccol.z()/255.f, 1.f), cstr.c_str());

        c_pos.x() += cstr.size() * char_inf::cwidth;

        if(i != (int)cols.size() - 1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

void render_copy_blind(vec3f col, const std::string& str, vec2f render_pos)
{
    ImGui::SetCursorScreenPos(ImVec2(render_pos.x(), render_pos.y()));

    ImGui::TextColored(ImVec4(col.x()/255.f, col.y()/255.f, col.z()/255.f, 1.f), str.c_str());
}

void imgui_render_str(sf::RenderWindow& win, const std::vector<formatted_char>& text, std::vector<std::vector<formatted_char>>& formatted_text)
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
        float width = ImGui::CalcTextSize(str.c_str(), nullptr, false, win.getSize().x).x;

        if(handle->held)
            render_copy_aware(win, col, str, spos, (vec2f){spos.x(), spos.y()} + (vec2f){width, 0.f}, pos);
        else
            render_copy_blind(col, str, pos);

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

void terminal_imgui::render(sf::RenderWindow& win)
{
    copy_handler* handle = get_global_copy_handler();

    std::vector<std::vector<formatted_char>> formatted_text;

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(win.getSize().x, win.getSize().y));

    ImGui::Begin("asdf1", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::BeginChild("left_sub", ImVec2(win.getSize().x - 40, 0.f), false, ImGuiWindowFlags_NoScrollbar);

    bool set_scroll = false;

    if(ImGui::IsWindowHovered() && scroll_hack.scrolled_this_frame != 0)
    {
        scroll_hack.scrolled += scroll_hack.scrolled_this_frame;
        scroll_hack.scrolled_this_frame = 0.f;
        set_scroll = true;
    }

    auto wrap_dim = ImGui::GetWindowSize();
    auto start = ImGui::GetWindowPos();

    wrap_dim.x += start.x;
    wrap_dim.y += start.y;

    vec2f current = {start.x, start.y};

    int vertical_columns = ceil((float)wrap_dim.y / char_inf::cheight);

    int min_start = (int)text_history.size() - vertical_columns;

    min_start = min_start - scroll_hack.scrolled;

    if(min_start < 0)
        min_start = 0;

    std::vector<std::vector<formatted_char>> formatted;

    auto all_interop = text_history;

    std::string render_command = command.command;
    bool specials = true;

    if(render_command == "")
    {
        render_command = "`bType something here...`";
        specials = false;
    }

    auto icommand = string_to_interop(render_command, specials, auto_handle, false);

    int cursor_offset = 0;

    auto_handle.handle_autocompletes(icommand, command.cursor_pos_idx, cursor_offset, command.command);

    interop_char curs;
    curs.col = {255, 255, 255};
    curs.c = '|';
    curs.is_cursor = true;

    int curs_cur = command.cursor_pos_idx + cursor_offset;

    if(focused)
    {
        if(curs_cur >= (int)icommand.size())
            icommand.push_back(curs);
        else if(curs_cur >= 0 && curs_cur < (int)icommand.size())
            icommand.insert(icommand.begin() + curs_cur, curs);
    }

    all_interop.push_back(icommand);

    for(int i=min_start; i < (int)all_interop.size(); i++)
    {
        formatted.push_back(format_characters(all_interop[i], current, {start.x, start.y}, (vec2f){wrap_dim.x, wrap_dim.y}, 0.f));
    }

    internally_format(formatted, {start.x, start.y + ImGui::GetWindowHeight()}, scroll_hack.scrolled * char_inf::cheight);

    for(auto& i : formatted)
    {
        imgui_render_str(win, i, formatted_text);
    }

    ImGui::EndChild();

    ImGui::SameLine(0.f, 0.f);

    ///rough
    scroll_hack.do_hack(win, all_interop.size() + 1, set_scroll);

    if(scroll_hack.scrolling)
    {
        scroll_hack.scrolled = (1.f - scroll_hack.output_scroll_frac) * (all_interop.size() + 1.f);
    }

    ImGui::End();

    handle->process_formatted(formatted_text);
}

void terminal_imgui::bump_command_to_history()
{
    text_history.push_back(string_to_interop(command.command, true, auto_handle));
    command.clear_command();
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
        std::string command_str = "command ";
        std::string chat_api = "chat_api ";
        std::string scriptargs = "server_scriptargs ";
        std::string invalid_str = "server_scriptargs_invalid";
        std::string ratelimit_str = "server_scriptargs_ratelimit ";

        if(command_info.type == server_command_command)
        {
            str = c_str_consume(sa_command_to_human_readable(command_info));

            push = true;
        }
        else if(command_info.type == server_command_chat_api)
        {
            std::vector<std::string> chnls;
            std::vector<std::string> msgs;

            std::vector<std::string> in_channels;

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

            sa_destroy_chat_api_info(chat_info);

            chat_win.set_side_channels(in_channels);

            for(int i=0; i < (int)chnls.size(); i++)
            {
                text_history.push_back(string_to_interop(msgs[i] + "\n", false, chat_win.auto_handle));

                chat_threads[chnls[i]].chats.push_back(string_to_interop(msgs[i], false, chat_win.auto_handle));
            }

            int max_history = 250;

            limit_size(text_history, max_history);
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
        else
        {
            push = true;
        }
    }
    else
    {
        push = true;
    }

    if(push)
    {
        int max_history = 250;

        text_history.push_back(string_to_interop(str, false, auto_handle));

        limit_size(text_history, max_history);
        consider_resetting_scrollbar = true;
    }

    sa_destroy_server_command_info(command_info);
}
