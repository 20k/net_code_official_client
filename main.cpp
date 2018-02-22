#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

#include "http_beast_client.hpp"

#include <windows.h>

#include "util.hpp"

#include <crapmud/script_util_shared.hpp>
#include "colour_interop.hpp"
#include "string_helpers.hpp"
#include <serialise/serialise.hpp>
#include <crapmud/shared_data.hpp>

struct chat_thread : serialisable
{
    std::vector<std::string> chats;

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

    std::vector<button> side_buttons
    {
        {"0000", true},
        {"7001"},
        {"memes"}
    };

    std::string selected = "0000";
    editable_string command;

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

        std::vector<int> specials;
        specials.resize(thread.chats.size());

        for(auto& i : specials)
            i = 0;

        int idx = command.cursor_pos_idx;

        if(!focused)
            idx = -1;

        ::render(win, command.command, thread.chats, specials, idx, {render_pos.x(), dim.y()}, {(int)win.getSize().x - char_inf::cwbuf/2.f - border_size, win.getSize().y}, border_size);

        render_side_attachment(win);
    }

    void render_side_attachment(sf::RenderWindow& win)
    {
        vec2f side_dim = {100, dim.y()};
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

            auto ichars = string_to_interop(side_buttons[i].txt, false);

            render_str(win, ichars, current_pos, start_pos, start_pos + side_dim, -1, border_size);

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
        return pos.x() >= render_start.x() && pos.y() >= render_start.y() &&
               pos.x() < render_start.x() + dim.x() && pos.y() < render_start.y() + dim.y();
    }
};

struct terminal : serialisable
{
    std::vector<std::string> text_history;
    std::vector<int> render_specials;

    std::map<std::string, chat_thread> chat_threads;

    sf::Font font;

    bool focused = true;
    editable_string command;

    virtual void do_serialise(serialise& s, bool ser)
    {
        s.handle_serialise(text_history, ser);
        s.handle_serialise(render_specials, ser);
        s.handle_serialise(chat_threads, ser);
        s.handle_serialise(command, ser);
    }

    terminal()
    {
        font.loadFromFile("VeraMono.ttf");
    }

    void render(sf::RenderWindow& win)
    {
        int idx = command.cursor_pos_idx;

        if(!focused)
            idx = -1;

        ::render(win, command.command, text_history, render_specials, idx, {0.f, win.getSize().y}, {(int)win.getSize().x - char_inf::cwbuf, win.getSize().y}, -char_inf::cheight);
    }

    void bump_command_to_history()
    {
        text_history.push_back(command.command);
        render_specials.push_back(1);
        command.clear_command();
    }

    void add_text_from_server(const std::string& in)
    {
        if(in == "")
            return;

        std::string command_str = "command ";
        std::string chat_api = "chat_api ";

        std::string str = in;

        if(str.substr(0, command_str.size()) == command_str)
        {
            str = std::string(str.begin() + command_str.size(), str.end());
        }
        else if(str.substr(0, chat_api.size()) == chat_api)
        {
            std::vector<std::string> strings = no_ss_split(str, " ");

            if(strings.size() >= 2)
            {
                //std::string channel = strings[1];

                std::string fchannel = "";
                int offset = 0;

                for(offset=chat_api.size(); offset < (int)str.size(); offset++)
                {
                    char c = str[offset];

                    if(c == ' ')
                        break;

                    fchannel += c;
                }

                offset++;

                std::string msg = "";

                for(; offset < (int)str.size(); offset++)
                {
                    msg += str[offset];
                }

                str = msg;

                std::cout << "fstr " << str << std::endl;
                std::cout << "fchn " << fchannel << std::endl;

                int max_chat_history = 500;

                limit_size(chat_threads[fchannel].chats, max_chat_history);

                chat_threads[fchannel].chats.push_back(str);
            }
        }

        int max_history = 1000;

        limit_size(text_history, max_history);
        limit_size(render_specials, max_history);

        text_history.push_back(str);
        render_specials.push_back(0);
    }
};

bool is_focused(sf::RenderWindow& win)
{
    return win.getSystemHandle() == GetFocus();
}

#define DMAP(A) key_map[sf::Keyboard::A] = tolower((#A)[0]);

int main()
{
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

    std::string terminal_file = "./terminal.txt";
    std::string chat_file = "./chat.txt";

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

    sf::Keyboard key;

    double diff_s = 0.f;

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
                }

                if(event.key.code == sf::Keyboard::Down)
                {
                    to_edit->move_command_history_idx(1);
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
        }

        if(ONCE_MACRO(sf::Keyboard::Return) && is_focused(window))
        {
            //term.add_to_command('\n');

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
                shared.add_back_write("client_command " + term.command.command);
            }
            else
            {
                ///TODO
                shared.add_back_write("client_chat #hs.chats.send({channel:\"" + chat_win.selected + "\", msg:\"" + chat_win.command.command + "\"})");
            }

            if(term.focused)
            {
                term.bump_command_to_history();
            }
            else
            {
                chat_win.command.clear_command();
            }

            serialise sterm;
            sterm.handle_serialise(term, true);
            sterm.save(terminal_file);

            serialise swindow;
            swindow.handle_serialise(chat_win, true);
            swindow.save(chat_file);
        }

        if(ONCE_MACRO(sf::Mouse::Left) && is_focused(window))
        {
            sf::Mouse mouse;
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
            term.add_text_from_server(shared.get_front_read());

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

        //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

        term.render(window);

        chat_win.render(window, term.chat_threads);

        window.display();
        window.clear(sf::Color(30, 30, 30));

        Sleep(4);
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
