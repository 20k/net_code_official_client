#include "ui_components.hpp"
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include <libncclient/nc_string_interop.hpp>
#include <libncclient/c_server_api.h>

void vertical_scrollbar::render(sf::RenderWindow& win, vec2f pos)
{

}

void vertical_scrollbar::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(current_viewed_line_number, ser);
}


#if 0
void terminal::do_serialise(serialise& s, bool ser)
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

void terminal::clear_terminal()
{
    text_history.clear();
}

void terminal::clear_chat()
{
    chat_threads.clear();
}

void terminal::clear_text()
{
    clear_terminal();
    clear_chat();
}

terminal::terminal()
{
    font.loadFromFile("VeraMono.ttf");

    auto_handle.use_autocomplete = true;
    auto_handle.use_autocolour = true;
}

void terminal::render(sf::RenderWindow& win)
{
    ::render(win, command.command, text_history, command.cursor_pos_idx, {0.f, win.getSize().y}, {(int)win.getSize().x - char_inf::cwbuf, win.getSize().y}, -char_inf::cheight, auto_handle, focused);
}

void terminal::bump_command_to_history()
{
    text_history.push_back(string_to_interop(command.command, true, auto_handle));
    command.clear_command();
}

void terminal::add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command)
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
#endif // 0
