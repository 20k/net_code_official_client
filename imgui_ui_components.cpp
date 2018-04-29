#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include <libncclient/nc_string_interop.hpp>
#include <libncclient/c_server_api.h>
#include "ui_components.hpp"
#include "copy_handler.hpp"

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
};

void render_copy_aware(sf::RenderWindow& win, vec3f col, const std::string& str, vec2f start_pos, vec2f end_pos)
{
    int cutoff_point = -1;

    std::vector<std::pair<std::string, vec3f>> cols;

    std::pair<std::string, vec3f> cur_str;

    if(str.size() == 0)
        return;

    copy_handler* handle = get_global_copy_handler();
    bool is_inside = handle->char_is_within_select_box(start_pos);

    for(int i=0; i < str.size(); i++)
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

    for(int i=0; i < (int)cols.size(); i++)
    {
        vec3f ccol = cols[i].second;
        const std::string& cstr = cols[i].first;

        ImGui::TextColored(ImVec4(ccol.x()/255.f, ccol.y()/255.f, ccol.z()/255.f, 1.f), cstr.c_str());

        if(i != (int)cols.size() - 1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

void imgui_render_str(sf::RenderWindow& win, const std::vector<interop_char>& text, std::vector<std::vector<formatted_char>>& formatted_text)
{
    copy_handler* handle = get_global_copy_handler();

    std::vector<render_command> commands;

    render_command current;

    for(const interop_char& fchar : text)
    {
        if(fchar.col != current.col || fchar.c == '\n')
        {
            if(current.str.size() > 0 && fchar.c != '\n')
            {
                current.type = imgui_text;
                commands.push_back(current);
                current = render_command();
            }

            if(fchar.c == '\n')
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
                continue;
            }
        }

        current.col = fchar.col;
        current.str += fchar.c;
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
            ImGui::Text("\n");
            continue;
        }

        std::string str = next.str;
        vec3f col = next.col;

        auto spos = ImGui::GetCursorScreenPos();

        ///need to predict here if the text is hilighted or not
        ///then if it is, replace spaces with "-" and colour blue
        float width = ImGui::CalcTextSize(str.c_str(), nullptr, false, win.getSize().x).x;

        if(handle->held)
            render_copy_aware(win, col, str, (vec2f){spos.x, spos.y}, (vec2f){spos.x, spos.y} + (vec2f){width, 0.f});
        else
            ImGui::TextColored(ImVec4(col.x()/255.f, col.y()/255.f, col.z()/255.f, 1.f), str.c_str());

        float x_start = spos.x;
        float x_end = spos.x + width;

        float y_coord = spos.y;

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

    ImGui::Begin("asdf", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoTitleBar);

    for(int i=0; i < (int)text_history.size(); i++)
    {
        imgui_render_str(win, text_history[i], formatted_text);
    }

    auto interop_cmd = string_to_interop_no_autos(command.command, true);

    imgui_render_str(win, interop_cmd, formatted_text);

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

        limit_size(text_history, max_history);

        text_history.push_back(string_to_interop(str, false, auto_handle));
    }

    sa_destroy_server_command_info(command_info);
}
