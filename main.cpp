#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

#include "http_beast_client.hpp"

#include <windows.h>

#include "util.hpp"

#include <crapmud/script_util_shared.hpp>
#include "colour_interop.hpp"

struct chat_thread
{
    std::vector<std::string> chats;
};

struct terminal
{
    std::string command;
    int command_history_idx = 0;
    std::vector<std::string> command_history;
    std::vector<std::string> text_history;
    std::vector<bool> render_specials;

    int cursor_pos_idx = 0;

    std::map<std::string, chat_thread> chat_threads;

    sf::Font font;

    terminal()
    {
        font.loadFromFile("VeraMono.ttf");
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

    bool should_send()
    {
        return command.size() > 0 && command.back() == '\n';
    }

    void remove_back()
    {
        if(command.size() > 0)
            command.pop_back();
    }

    int cwidth = 8;

    int cheight = 16;
    int cwbuf = 4;

    int get_num_lines(sf::RenderWindow& win, const std::vector<interop_char>& str)
    {
        int width = win.getSize().x;

        int startx = cwbuf;
        int num_lines = 0;

        for(int i=0; i < (int)str.size(); i++)
        {
            startx += cwidth;

            if(str[i].c == '\n')
            {
                num_lines++;
                startx = cwbuf;
                continue;
            }

            if(startx >= width - cwbuf)
            {
                num_lines++;
                startx = cwbuf;
                continue;
            }
        }

        return num_lines;
    }

    void render_str(sf::RenderWindow& win, const std::string& str, vec2f& cpos, bool render_specials)
    {
        sf::Text txt;
        txt.setFont(font);

        vec2f pos = cpos;

        std::vector<interop_char> chars = build_from_colour_string(str, render_specials);

        while(chars.size() > 0 && chars.back().c == '\n')
        {
            chars.pop_back();
        }

        int num_lines = get_num_lines(win, chars);

        chars.push_back({'\n'});

        pos.y() -= num_lines * cheight;

        for(int i=0; i < chars.size(); i++)
        {
            if(pos.x() >= ((int)win.getSize().x) - cwbuf || chars[i].c == '\n')
            {
                pos.y() += cheight;
                pos.x() = cwbuf;
            }

            if(chars[i].c == '\n')
                continue;

            vec2f found_pos = round(pos);

            txt.setString(std::string(1, chars[i].c));
            txt.setCharacterSize(12);
            txt.setPosition(found_pos.x(), found_pos.y());

            vec3f col = chars[i].col;

            txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));

            //vec2i dim = {txt.getGlobalBounds().width, txt.getGlobalBounds().height};

            pos.x() += cwidth;

            win.draw(txt);
        }

        cpos.y() -= num_lines * cheight;
    }

    void render(sf::RenderWindow& win)
    {
        vec2f start_pos = {cwbuf, win.getSize().y - cheight};
        vec2f current_pos = start_pos;

        render_str(win, command, current_pos, true);

        current_pos.y() -= cheight;

        int len = text_history.size();

        for(int i=len-1; i >= 0; i--)
        {
            std::string str = text_history[i];

            if(current_pos.y() >= win.getSize().y || current_pos.y() + cheight < 0)
                continue;

            render_str(win, str, current_pos, render_specials[i]);

            current_pos.y() -= cheight;
        }

        std::string cursor_icon = "|";

        vec2f to_render_curs = start_pos;
        to_render_curs.x() += cwidth * cursor_pos_idx - cwidth/2.f;

        render_str(win, cursor_icon, to_render_curs, false);
    }

    void move_command_history_idx(int dir)
    {
        command_history_idx += dir;

        command_history_idx = clamp(command_history_idx, 0, (int)command_history.size());

        if(command_history_idx >= 0 && command_history_idx < command_history.size())
        {
            command = command_history[command_history_idx];
        }

        if(command_history_idx == command_history.size())
        {
            ///ideally we'd reset to partially held commands
            command = "";
        }

        cursor_pos_idx = command.size();
    }

    void move_cursor(int dir)
    {
        cursor_pos_idx += dir;
        cursor_pos_idx = clamp(cursor_pos_idx, 0, (int)command.size());
    }

    void process_backspace()
    {
        int to_remove = cursor_pos_idx - 1;

        if(to_remove < 0 || to_remove >= command.size())
            return;

        command.erase(command.begin() + to_remove);

        cursor_pos_idx--;
    }

    void process_delete()
    {
        int to_remove = cursor_pos_idx;

        if(to_remove < 0 || to_remove >= command.size())
            return;

        command.erase(command.begin() + to_remove);
    }

    void clear_command()
    {
        command = "";
        command_history_idx = command_history.size();
        cursor_pos_idx = 0;
    }

    void push_command_to_history(const std::string& cmd)
    {
        command_history.push_back(cmd);
        command_history_idx = (int)command_history.size();
    }

    void bump_command_to_history()
    {
        text_history.push_back(command);
        render_specials.push_back(1);
        clear_command();
    }

    void add_text_from_server(const std::string& in)
    {
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

                for(offset=chat_api.size(); offset < str.size(); offset++)
                {
                    char c = str[offset];

                    if(c == ' ')
                        break;

                    fchannel += c;
                }

                offset++;

                std::string msg = "";

                for(; offset < str.size(); offset++)
                {
                    msg += str[offset];
                }

                str = msg;

                std::cout << "fstr " << str << std::endl;
                std::cout << "fchn " << fchannel << std::endl;

                chat_threads[fchannel].chats.push_back(str);
            }
        }

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

        //shared.add_back_write("auth client " + shared.auth);
    }

    test_http_client(shared);

    sf::ContextSettings sett;
    sett.antialiasingLevel = 8;

    sf::RenderWindow window;
    window.create(sf::VideoMode(800,600), "Crapmud", sf::Style::Default, sett);

    terminal term;

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

    sf::Keyboard key;

    bool running = true;

    while(running)
    {
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
                        term.add_to_command(event.text.unicode);
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
                    term.process_backspace();
                }

                if(event.key.code == sf::Keyboard::Delete)
                {
                    term.process_delete();
                }

                if(event.key.code == sf::Keyboard::Up)
                {
                    term.move_command_history_idx(-1);
                }

                if(event.key.code == sf::Keyboard::Down)
                {
                    term.move_command_history_idx(1);
                }

                if(event.key.code == sf::Keyboard::Left)
                {
                    term.move_cursor(-1);
                }

                if(event.key.code == sf::Keyboard::Right)
                {
                    term.move_cursor(1);
                }

                if(event.key.code == sf::Keyboard::Escape)
                {
                    term.clear_command();
                }
            }
        }

        if(ONCE_MACRO(sf::Keyboard::Return) && is_focused(window))
        {
            //term.add_to_command('\n');

            term.command = strip_whitespace(term.command);

            term.push_command_to_history(term.command);

            std::string swapping_users = "user ";

            if(term.command.substr(0, swapping_users.length()) == swapping_users)
            {
                std::vector<std::string> spl = no_ss_split(term.command, " ");

                ///HACK ALERT
                ///NEED TO WAIT FOR SERVER CONFIRMATION
                if(spl.size() >= 2)
                {
                    shared.set_user(spl[1]);
                }
            }

            shared.add_back_write(term.command);
            term.bump_command_to_history();
        }

        if(shared.has_front_read())
        {
            term.add_text_from_server(shared.get_front_read());
        }

        term.render(window);

        window.display();
        window.clear(sf::Color(30, 30, 30));

        Sleep(4);
    }

    return 0;
}
