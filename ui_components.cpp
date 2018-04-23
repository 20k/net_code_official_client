#include "ui_components.hpp"
#include "string_helpers.hpp"
#include <crapmud/script_util_shared.hpp>

void chat_thread::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(chats, ser);
}

button::button() {}
button::button(const std::string& txt, bool is_selected) : txt(txt), is_selected(is_selected) {}

bool button::within(vec2f mpos)
{
    return mpos.x() >= pos.x() && mpos.y() >= pos.y() && mpos.x() < pos.x() + dim.x() && mpos.y() < pos.y() + dim.y();
}

void button::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(txt, ser);
    s.handle_serialise(is_selected, ser);
    s.handle_serialise(pos, ser);
    s.handle_serialise(dim, ser);
}

void chat_window::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(render_start, ser);
    s.handle_serialise(side_buttons, ser);
    s.handle_serialise(selected, ser);
    s.handle_serialise(command, ser);
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
    for(auto& i : side_buttons)
    {
        if(i.txt == selected)
        {
            i.is_selected = true;
        }
        else
        {
            i.is_selected = false;
        }
    }
}

void chat_window::render(sf::RenderWindow& win, std::map<std::string, chat_thread>& threads)
{
    vec2f swidth = {win.getSize().x, win.getSize().y};

    vec2f render_pos = (vec2f) {swidth.x() - dim.x() - border_size, border_size};
    render_start = render_pos;

    sf::RectangleShape shape;

    shape.setSize({dim.x(), dim.y()});
    shape.setOutlineColor(get_frame_col());
    shape.setOutlineThickness(border_size);
    shape.setFillColor(sf::Color(30,30,30,255));

    shape.setPosition(render_pos.x(), render_pos.y());

    win.draw(shape);

    chat_thread& thread = threads[selected];

    /*std::vector<int> specials;
    specials.resize(thread.chats.size());

    for(auto& i : specials)
        i = 0;*/

    ::render(win, command.command, thread.chats, command.cursor_pos_idx, {render_pos.x(), dim.y()}, {(int)win.getSize().x - char_inf::cwbuf/2.f - border_size, win.getSize().y}, border_size, auto_handle, focused);

    render_side_attachment(win);
}

void chat_window::render_side_attachment(sf::RenderWindow& win)
{
    vec2f side_pos = {render_start.x() - side_dim.x() - border_size, render_start.y()};

    sf::RectangleShape shape;

    shape.setSize({side_dim.x(), side_dim.y()});
    shape.setOutlineColor(get_frame_col());
    shape.setOutlineThickness(border_size);
    shape.setFillColor(sf::Color(30, 30, 30, 255));

    shape.setPosition(side_pos.x(), side_pos.y());

    win.draw(shape);

    vec2f start_pos = {side_pos.x() + char_inf::cwbuf, side_pos.y() + char_inf::cheight/4.f};
    vec2f current_pos = start_pos;

    for(int i=0; i < (int)side_buttons.size(); i++)
    {
        side_buttons[i].pos = current_pos;
        side_buttons[i].dim = {side_dim.x(), char_inf::cheight};

        if(side_buttons[i].is_selected)
        {
            sf::RectangleShape shape;
            shape.setPosition({current_pos.x() - char_inf::cwbuf, current_pos.y()});
            shape.setFillColor(get_highlight_col());

            vec2f cdim = side_buttons[i].dim;
            //cdim.x() -= char_inf::cwbuf;

            shape.setSize({cdim.x(), cdim.y()});

            win.draw(shape);
        }

        auto ichars = string_to_interop(side_buttons[i].txt, false, auto_handle);

        render_str(win, ichars, current_pos, start_pos, start_pos + side_dim, border_size);

        current_pos.y() += char_inf::cheight;
    }
}

void chat_window::process_click(vec2f pos)
{
    bool any = false;

    for(button& b : side_buttons)
    {
        if(b.within(pos))
        {
            any = true;
            break;
        }
    }

    if(!any)
        return;

    for(button& b : side_buttons)
    {
        if(b.within(pos))
        {
            selected = b.txt;
            b.is_selected = true;
        }
        else
        {
            b.is_selected = false;
        }
    }
}

bool chat_window::within(vec2f pos)
{
    vec2f tl = render_start - side_dim;
    vec2f br = render_start + (vec2f) {dim.x(), dim.y()};

    return pos.x() >= tl.x() && pos.y() >= tl.y() &&
           pos.x() < br.x() && pos.y() < br.y();
}

void chat_window::set_side_channels(const std::vector<std::string>& sides)
{
    side_buttons.clear();

    for(auto& i : sides)
    {
        if(i == selected)
            side_buttons.push_back({i, true});
        else
            side_buttons.push_back({i});
    }
}

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

void terminal::get_chat_api_strs(const std::string& chat_in, std::vector<std::string>& channels, std::vector<std::string>& msgs, std::vector<std::string>& in_channels)
{
    std::string chat_api = "chat_api ";

    //std::cout << chat_in << std::endl;

    auto post_intro = chat_in.begin() + chat_api.size();

    auto strs = no_ss_split(chat_in, " ");

    if(strs.size() < 3)
        return;

    std::string prologue_size = strs[1];
    std::string num_channels = strs[2];

    int num = atoi(num_channels.c_str());
    int prologue_bytes = atoi(prologue_size.c_str());

    int base = 3;

    for(int i=0; i < num; i++)
    {
        int offset = i + base;

        std::string user_is_in_chan = strs[offset];

        in_channels.push_back(user_is_in_chan);

        //std::cout << user_is_in_chan << " fchan " << std::endl;
    }

    std::string remaining(post_intro + prologue_bytes + prologue_size.size() + 1, chat_in.end());

    if(remaining.size() > 0 && remaining.front() == ' ')
        remaining.erase(remaining.begin());

    while(1)
    {
        //std::cout << "rem " << remaining << std::endl;

        auto bytes_check = no_ss_split(remaining, " ");

        if(bytes_check.size() == 0)
            return;

        int next_size = atoi(bytes_check[0].c_str());

        auto it = remaining.begin();

        while(*it != ' ')
            it++;

        it++;

        if(next_size == 0)
        {
            it++;

            if(it >= remaining.end())
                return;

            remaining = std::string(it, remaining.end());

            continue;
        }

        std::string total_msg(it, it + next_size);

        auto next_it = it;

        while(*next_it != ' ')
            next_it++;

        std::string chan(it, next_it);

        next_it++;

        std::string msg(next_it, it + next_size);

        channels.push_back(chan);
        msgs.push_back(msg);

        if(it + next_size >= remaining.end())
            return;

        remaining = std::string(it + next_size, remaining.end());
    }
}

void terminal::parse_scriptargs(const std::string& in)
{
    std::cout << "pstring " << in << std::endl;

    ///TODO: Make all server/client communication use this format
    std::string_view view(&in[0]);

    view.remove_prefix(std::min(view.find_first_of(" ")+1, view.size()));

    std::vector<std::string> strings;

    while(view.size() > 0)
    {
        auto found = view.find(" ");

        if(found == std::string_view::npos)
            break;

        std::string_view num(view.data(), found);

        std::string len(num);

        int ilen = stoi(len);

        std::string dat(view.substr(len.size() + 1, ilen));

        strings.push_back(dat);

        ///"len" + " " + len_bytes + " "
        view.remove_prefix(len.size() + 2 + ilen);
    }

    if(strings.size() == 0)
        return;

    std::string scriptname = strings[0];

    strings.erase(strings.begin());

    /*std::cout << "for script " << scriptname << std::endl;

    for(auto& i : strings)
    {
        std::cout << i << std::endl;
    }*/

    std::vector<autocomplete_args> args;

    int len = strings.size();

    if((strings.size() % 2) != 0)
        return;

    for(int i=0; i < (int)strings.size(); i+=2)
    {
        std::string key = strings[i];
        std::string arg = strings[i + 1];

        args.push_back({key, arg});
    }

    ///this all fundamentally works

    ///to future james
    ///dump script in here
    ///dump args
    ///when we detect a fullname script in terminal with args, show in gray
    ///press tab to fill in and jump cursor between args
    ///to future james again
    ///gotta hardcode in autocomplete args to trust scripts (limitation of c++ style), maybe some sort of REGISTER_ARGS thing
    auto_handle.found_args[scriptname] = args;
    auto_handle.is_valid[scriptname] = true;
}

void terminal::add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command)
{
    if(in == "")
        return;

    std::string str = in;

    if(server_command)
    {
        std::string command_str = "command ";
        std::string chat_api = "chat_api ";
        std::string scriptargs = "server_scriptargs ";
        std::string invalid_str = "server_scriptargs_invalid";
        std::string ratelimit_str = "server_scriptargs_ratelimit ";

        if(starts_with(str, command_str))
        {
            str = std::string(str.begin() + command_str.size(), str.end());
        }
        else if(starts_with(str, chat_api))
        {
            std::vector<std::string> chnls;
            std::vector<std::string> msgs;

            std::vector<std::string> in_channels;

            get_chat_api_strs(str, chnls, msgs, in_channels);

            chat_win.set_side_channels(in_channels);

            for(int i=0; i < (int)chnls.size(); i++)
            {
                text_history.push_back(string_to_interop(msgs[i] + "\n", false, chat_win.auto_handle));

                chat_threads[chnls[i]].chats.push_back(string_to_interop(msgs[i], false, chat_win.auto_handle));
            }

            int max_history = 250;

            limit_size(text_history, max_history);

            return;
        }
        else if(starts_with(str, scriptargs))
        {
            //std::cout << str << std::endl;

            parse_scriptargs(str);

            return;
        }
        else if(starts_with(str, invalid_str))
        {
            if(starts_with(str, invalid_str + " "))
            {
                std::string script(in.begin() + invalid_str.size() + 1, in.end());

                if(script.size() > 0)
                {
                    auto_handle.is_valid[script] = false;
                }
            }

            return;
        }
        else if(starts_with(str, ratelimit_str))
        {
            std::string script(in.begin() + ratelimit_str.size(), in.end());

            if(script.size() > 0)
            {
                auto_handle.found_unprocessed_autocompletes.insert(script);
            }

            return;
        }
    }

    int max_history = 250;

    limit_size(text_history, max_history);

    text_history.push_back(string_to_interop(str, false, auto_handle));
}
