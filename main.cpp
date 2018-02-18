#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

#include "http_beast_client.hpp"

#include <windows.h>

#include "util.hpp"

#include <crapmud/script_util_shared.hpp>
#include "colour_interop.hpp"

struct terminal
{
    std::string command;
    int command_history_idx = 0;
    std::vector<std::string> command_history;
    std::vector<std::string> text_history;

    sf::Font font;

    terminal()
    {
        font.loadFromFile("VeraMono.ttf");
    }

    void add_to_command(char c)
    {
        command.push_back(c);
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

    int get_num_lines(sf::RenderWindow& win, int str_len)
    {
        int len = str_len * cwidth;

        int num_lines = floor(len / (float)win.getSize().x);

        return num_lines;
    }

    ///need to wrap text here
    void render_str(sf::RenderWindow& win, const std::string& str, vec2f& cpos)
    {
        sf::Text txt;
        txt.setFont(font);

        vec2f pos = cpos;

        std::vector<interop_char> chars = build_from_colour_string(str);

        int num_lines = get_num_lines(win, chars.size());

        pos.y() -= num_lines * cheight;

        for(int i=0; i < chars.size(); i++)
        {
            if(pos.x() >= win.getSize().x - 4)
            {
                pos.y() += cheight;
                pos.x() = cwbuf;
            }

            vec2f found_pos = round(pos);

            txt.setString(std::string(1, chars[i].c));
            txt.setCharacterSize(12);
            txt.setPosition(found_pos.x(), found_pos.y());

            vec3f col = chars[i].col;

            txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));

            vec2i dim = {txt.getGlobalBounds().width, txt.getGlobalBounds().height};

            pos.x() += 8;

            win.draw(txt);
        }

        cpos.y() -= num_lines * cheight;
    }

    void render(sf::RenderWindow& win)
    {
        vec2f start_pos = {cwbuf, win.getSize().y - cheight};
        vec2f current_pos = start_pos;

        render_str(win, command, current_pos);

        current_pos.y() -= cheight;

        int len = text_history.size();

        for(int i=len-1; i >= 0; i--)
        {
            std::string str = text_history[i];

            render_str(win, str, current_pos);

            current_pos.y() -= cheight;
        }
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
    }

    void clear_command()
    {
        command = "";
        command_history_idx = command_history.size();
    }

    void push_command_to_history(const std::string& cmd)
    {
        command_history.push_back(cmd);
        command_history_idx = (int)command_history.size();
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

            if(event.type == sf::Event::KeyPressed)
            {
                if(event.key.code == sf::Keyboard::BackSpace)
                {
                    term.remove_back();
                }

                if(event.key.code == sf::Keyboard::Up)
                {
                    term.move_command_history_idx(-1);
                }

                if(event.key.code == sf::Keyboard::Down)
                {
                    term.move_command_history_idx(1);
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
            term.text_history.push_back(term.command);
            term.command = "";
        }

        if(shared.has_front_read())
        {
            term.text_history.push_back(shared.get_front_read());
        }

        term.render(window);

        window.display();
        window.clear(sf::Color(30, 30, 30));
    }

    return 0;
}
