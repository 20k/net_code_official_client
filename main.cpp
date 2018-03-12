#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

#include "http_beast_client.hpp"

#include "util.hpp"

#include <crapmud/script_util_shared.hpp>
#include "colour_interop.hpp"
#include "string_helpers.hpp"
#include <serialise/serialise.hpp>
#include <crapmud/shared_data.hpp>

#include "local_commands.hpp"
#include "auto_handlers.hpp"
#include "copy_handler.hpp"
#include "stacktrace.hpp"

struct chat_thread : serialisable
{
    std::vector<interop_vec_t> chats;

    virtual void do_serialise(serialise& s, bool ser)
    {
        s.handle_serialise(chats, ser);
    }
};

template<typename T>
void limit_size(T& t, int max_size)
{
    while((int)t.size() >= max_size)
    {
        t.erase(t.begin());
    }
}

struct editable_string : serialisable
{
    int cursor_pos_idx = 0;
    std::string command;
    std::vector<std::string> command_history;
    int command_history_idx = 0;

    virtual void do_serialise(serialise& s, bool ser)
    {
        s.handle_serialise(cursor_pos_idx, ser);
        s.handle_serialise(command, ser);
        s.handle_serialise(command_history, ser);
        s.handle_serialise(command_history_idx, ser);
    }

    void add_to_command(char c)
    {
        if(cursor_pos_idx >= (int)command.size())
        {
            command.push_back(c);
        }
        else
        {
            command.insert(command.begin() + cursor_pos_idx, c);
        }

        cursor_pos_idx++;
    }

    void remove_back()
    {
        if(command.size() > 0)
            command.pop_back();
    }

    void move_cursor(int dir)
    {
        cursor_pos_idx += dir;
        cursor_pos_idx = clamp(cursor_pos_idx, 0, (int)command.size());
    }

    void process_backspace()
    {
        int to_remove = cursor_pos_idx - 1;

        if(to_remove < 0 || to_remove >= (int)command.size())
            return;

        command.erase(command.begin() + to_remove);

        cursor_pos_idx--;
    }

    void process_delete()
    {
        int to_remove = cursor_pos_idx;

        if(to_remove < 0 || to_remove >= (int)command.size())
            return;

        command.erase(command.begin() + to_remove);
    }


    void move_command_history_idx(int dir)
    {
        command_history_idx += dir;

        command_history_idx = clamp(command_history_idx, 0, (int)command_history.size());

        if(command_history_idx >= 0 && command_history_idx < (int)command_history.size())
        {
            command = command_history[command_history_idx];
        }

        if(command_history_idx == (int)command_history.size())
        {
            ///ideally we'd reset to partially held commands
            command = "";
        }

        cursor_pos_idx = command.size();
    }

    void clear_command()
    {
        command = "";
        command_history_idx = command_history.size();
        cursor_pos_idx = 0;
    }

    void push_command_to_history(const std::string& cmd)
    {
        int max_command_history = 1000;

        limit_size(command_history, max_command_history);

        command_history.push_back(cmd);
        command_history_idx = (int)command_history.size();
    }
};

struct button : serialisable
{
    std::string txt;
    bool is_selected = false;

    vec2f pos;
    vec2f dim;

    button(){}
    button(const std::string& txt, bool is_selected = false) : txt(txt), is_selected(is_selected) {}

    bool within(vec2f mpos)
    {
        return mpos.x() >= pos.x() && mpos.y() >= pos.y() && mpos.x() < pos.x() + dim.x() && mpos.y() < pos.y() + dim.y();
    }

    virtual void do_serialise(serialise& s, bool ser)
    {
        s.handle_serialise(txt, ser);
        s.handle_serialise(is_selected, ser);
        s.handle_serialise(pos, ser);
        s.handle_serialise(dim, ser);
    }
};

struct chat_window : serialisable
{
    vec2f render_start = {0,0};
    vec2i dim = {500, 300};
    vec3f frame_col = {0.46f, 0.8f, 1.f};

    vec2f side_dim = {100, dim.y()};

    std::vector<button> side_buttons
    {
        {"0000"},
        {"7001"},
        {"memes"}
    };

    std::string selected = "0000";
    editable_string command;

    auto_handler auto_handle;

    virtual void do_serialise(serialise& s, bool ser)
    {
        s.handle_serialise(render_start, ser);
        s.handle_serialise(side_buttons, ser);
        s.handle_serialise(selected, ser);
        s.handle_serialise(command, ser);
    }

    sf::Color get_frame_col()
    {
        return sf::Color(frame_col.x()*255.f, frame_col.y()*255.f, frame_col.z()*255.f, 255);
    }

    sf::Color get_highlight_col()
    {
        return sf::Color(70, 70, 70, 255);
    }

    bool focused = false;
    float border_size = 2.f;

    void tick()
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

    void render(sf::RenderWindow& win, std::map<std::string, chat_thread>& threads)
    {
        vec2f swidth = {win.getSize().x, win.getSize().y};

        vec2f render_pos = (vec2f){swidth.x() - dim.x() - border_size, border_size};
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

    void render_side_attachment(sf::RenderWindow& win)
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

    void process_click(vec2f pos)
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

    bool within(vec2f pos)
    {
        vec2f tl = render_start - side_dim;
        vec2f br = render_start + (vec2f){dim.x(), dim.y()};

        return pos.x() >= tl.x() && pos.y() >= tl.y() &&
               pos.x() < br.x() && pos.y() < br.y();
    }

    void set_side_channels(const std::vector<std::string>& sides)
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
};

struct terminal : serialisable
{
    std::vector<interop_vec_t> text_history;

    std::map<std::string, chat_thread> chat_threads;

    sf::Font font;

    bool focused = true;
    editable_string command;

    auto_handler auto_handle;

    virtual void do_serialise(serialise& s, bool ser)
    {
        if(ser == false)
        {
            auto_handle.found_unprocessed_autocompletes.clear();
            auto_handle.found_args.clear();
        }

        s.handle_serialise(text_history, ser);
        s.handle_serialise(chat_threads, ser);
        s.handle_serialise(command, ser);
        s.handle_serialise_no_clear(auto_handle, ser);

        //std::cout << "loaded hist " << text_history.size() << std::endl;
    }

    terminal()
    {
        font.loadFromFile("VeraMono.ttf");

        auto_handle.use_autocomplete = true;
        auto_handle.use_autocolour = true;
    }

    void render(sf::RenderWindow& win)
    {
        ::render(win, command.command, text_history, command.cursor_pos_idx, {0.f, win.getSize().y}, {(int)win.getSize().x - char_inf::cwbuf, win.getSize().y}, -char_inf::cheight, auto_handle, focused);
    }

    void bump_command_to_history()
    {
        text_history.push_back(string_to_interop(command.command, true, auto_handle));
        command.clear_command();
    }

    void get_chat_api_strs(const std::string& chat_in, std::vector<std::string>& channels, std::vector<std::string>& msgs, std::vector<std::string>& in_channels)
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

    void parse_scriptargs(const std::string& in)
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
    }

    void add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command = true)
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
                        auto_handle.found_args[script] = {{"", "", false}};
                }

                return;
            }
        }

        int max_history = 250;

        limit_size(text_history, max_history);

        text_history.push_back(string_to_interop(str, false, auto_handle));
    }
};

bool is_focused(sf::RenderWindow& win)
{
    return win.getSystemHandle() == GetFocus();
}

#define DMAP(A) key_map[sf::Keyboard::A] = tolower((#A)[0]);

///test new repo
int main()
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    stack_on_start();

    shared_data shared;

    if(file_exists("key.key"))
    {
        shared.auth = read_file_bin("key.key");

        std::cout << "loaded auth of " << shared.auth.size() << std::endl;

        //shared.send_auth = true;
    }

    font.loadFromFile("VeraMono.ttf");

    test_http_client(shared);

    sf::ContextSettings sett;
    sett.antialiasingLevel = 8;

    sf::RenderWindow window;
    window.create(sf::VideoMode(1200,600), "Crapmud", sf::Style::Default, sett);

    terminal term;
    chat_window chat_win;

    sf::Event event;

    std::map<sf::Keyboard::Key, char> key_map;

    DMAP(A);DMAP(B);DMAP(C);
    DMAP(D);DMAP(E);DMAP(F);
    DMAP(G);DMAP(H);DMAP(I);
    DMAP(J);DMAP(K);DMAP(L);
    DMAP(M);DMAP(N);DMAP(O);
    DMAP(P);DMAP(Q);DMAP(R);
    DMAP(S);DMAP(T);DMAP(U);
    DMAP(V);DMAP(W);DMAP(X);
    DMAP(Y);DMAP(Z);

    std::string terminal_file = "./terminal_v3.txt";
    std::string chat_file = "./chat_v3.txt";

    if(file_exists(terminal_file))
    {
        serialise sterm;
        sterm.load(terminal_file);
        sterm.handle_serialise(term, false);
    }

    if(file_exists(chat_file))
    {
        serialise swindow;
        swindow.load(chat_file);
        swindow.handle_serialise(chat_win, false);
    }

    sf::Clock render_clock;

    sf::Clock client_poll_clock;

    sf::Clock request_clock;

    sf::Keyboard key;
    sf::Mouse mouse;

    //double diff_s = 0.f;

    bool running = true;

    while(running)
    {
        editable_string* to_edit = &term.command;

        if(chat_win.focused)
            to_edit = &chat_win.command;

        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
            {
                running = false;
            }

            if(event.type == sf::Event::TextEntered)
            {
                if(event.text.unicode < 128)
                {
                    if(event.text.unicode >= 32 && event.text.unicode <= 126)
                    {
                        to_edit->add_to_command(event.text.unicode);
                    }
                }
            }

            if(event.type == sf::Event::Resized)
            {
                window.setSize({event.size.width, event.size.height});
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
            }

            if(event.type == sf::Event::KeyPressed)
            {
                if(event.key.code == sf::Keyboard::BackSpace)
                {
                    to_edit->process_backspace();
                }

                if(event.key.code == sf::Keyboard::Delete)
                {
                    to_edit->process_delete();
                }

                if(event.key.code == sf::Keyboard::Up)
                {
                    to_edit->move_command_history_idx(-1);

                    term.auto_handle.clear_internal_state();
                }

                if(event.key.code == sf::Keyboard::Down)
                {
                    to_edit->move_command_history_idx(1);

                    term.auto_handle.clear_internal_state();
                }

                if(event.key.code == sf::Keyboard::Left)
                {
                    to_edit->move_cursor(-1);
                }

                if(event.key.code == sf::Keyboard::Right)
                {
                    to_edit->move_cursor(1);
                }

                if(event.key.code == sf::Keyboard::Escape)
                {
                    to_edit->clear_command();

                    term.auto_handle.clear_internal_state();
                }

                if(event.key.code == sf::Keyboard::V)
                {
                    if(key.isKeyPressed(sf::Keyboard::LControl) && is_focused(window))
                    {
                        std::string add_text = get_clipboard_contents();

                        for(auto& i : add_text)
                        {
                            to_edit->add_to_command(i);
                        }
                    }
                }
            }

            if(event.type == sf::Event::MouseButtonPressed)
            {
                if(event.mouseButton.button == sf::Mouse::Left)
                {
                    get_global_copy_handler()->on_lclick({event.mouseButton.x, event.mouseButton.y});
                }
            }

            if(event.type == sf::Event::MouseButtonReleased)
            {
                if(event.mouseButton.button == sf::Mouse::Left)
                {
                    get_global_copy_handler()->on_lclick_release({event.mouseButton.x, event.mouseButton.y});
                }
            }
        }

        if(ONCE_MACRO(sf::Keyboard::Return) && is_focused(window))
        {
            //term.add_to_command('\n');

            if(!chat_win.focused)
                to_edit->command = strip_whitespace(to_edit->command);

            to_edit->push_command_to_history(to_edit->command);

            std::string swapping_users = "user ";

            if(term.focused && term.command.command.substr(0, swapping_users.length()) == swapping_users)
            {
                std::vector<std::string> spl = no_ss_split(term.command.command, " ");

                ///HACK ALERT
                ///NEED TO WAIT FOR SERVER CONFIRMATION
                if(spl.size() >= 2)
                {
                    shared.set_user(spl[1]);
                }
            }

            if(term.focused)
            {
                if(!is_local_command(term.command.command))
                    shared.add_back_write("client_command " + term.command.command);

                term.auto_handle.clear_internal_state();
            }
            else
            {
                ///TODO
                shared.add_back_write("client_chat #hs.msg.send({channel:\"" + chat_win.selected + "\", msg:\"" + chat_win.command.command + "\"})");
            }

            std::string cmd = term.command.command;

            if(term.focused)
            {
                term.bump_command_to_history();
            }
            else
            {
                chat_win.command.clear_command();
            }

            if(term.focused && is_local_command(cmd))
            {
                std::string data = handle_local_command(shared.get_user(), cmd);

                term.add_text_from_server(data, chat_win, false);
            }

            serialise sterm;
            sterm.handle_serialise(term, true);
            sterm.save(terminal_file);

            serialise swindow;
            swindow.handle_serialise(chat_win, true);
            swindow.save(chat_file);
        }

        chat_win.tick();

        auto sf_mpos = mouse.getPosition(window);
        vec2f vpos = {sf_mpos.x, sf_mpos.y};

        if(mouse.isButtonPressed(sf::Mouse::Left) && is_focused(window))
            get_global_copy_handler()->on_hold_lclick(window,  vpos);

        if(ONCE_MACRO(sf::Mouse::Left) && is_focused(window))
        {
            auto ppos = mouse.getPosition(window);

            vec2f mpos = {ppos.x, ppos.y};

            if(chat_win.within(mpos))
            {
                chat_win.focused = true;
                term.focused = false;
            }
            else
            {
                chat_win.focused = false;
                term.focused = true;
            }

            chat_win.process_click(mpos);
        }

        if(shared.has_front_read())
        {
            term.add_text_from_server(shared.get_front_read(), chat_win);

            serialise sterm;
            sterm.handle_serialise(term, true);
            sterm.save(terminal_file);

            serialise swindow;
            swindow.handle_serialise(chat_win, true);
            swindow.save(chat_file);
        }

        if(client_poll_clock.getElapsedTime().asSeconds() > 1)
        {
            shared.add_back_write("client_poll");

            client_poll_clock.restart();
        }

        ///hmm
        ///this is inadequate
        ///we need to be able to request multiple scripts at once
        ///and receive multiple as well
        if(term.auto_handle.found_unprocessed_autocompletes.size() > 0 && request_clock.getElapsedTime().asMilliseconds() > 200)
        {
            request_clock.restart();

            for(auto& str : term.auto_handle.found_unprocessed_autocompletes)
            {
                std::string command = "client_scriptargs " + str;

                shared.add_back_write(command);

                //std::cout << "requesting " << command << std::endl;

                break;
            }

            if(term.auto_handle.found_unprocessed_autocompletes.size() > 0)
                term.auto_handle.found_unprocessed_autocompletes.erase(term.auto_handle.found_unprocessed_autocompletes.begin());

            //term.auto_handle.found_unprocessed_autocompletes.clear();
        }

        std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

        term.auto_handle.window_in_focus = is_focused(window);
        chat_win.auto_handle.window_in_focus = is_focused(window);

        term.render(window);
        chat_win.render(window, term.chat_threads);

        ///this is a hack to fix the fact that sometimes
        ///click input doesn't make clean click/release pairs
        if(!mouse.isButtonPressed(sf::Mouse::Left))
        {
            get_global_copy_handler()->finished = false;
            get_global_copy_handler()->held = false;
        }

        window.display();
        window.clear(sf::Color(30, 30, 30));

        sf::sleep(sf::milliseconds(4));
    }

    serialise sterm;
    sterm.handle_serialise(term, true);
    sterm.save(terminal_file);

    serialise swindow;
    swindow.handle_serialise(chat_win, true);
    swindow.save(chat_file);

    shared.should_terminate = true;

    while(shared.termination_count != 3) {}

    return 0;
}
