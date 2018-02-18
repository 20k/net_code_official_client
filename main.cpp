#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

#include "http_beast_client.hpp"

#include <windows.h>

#include "util.hpp"

#include <crapmud/script_util_shared.hpp>

/*void render_font(sf::RenderWindow& win, const std::string& to_render, vec2f pos, vec4f col, float scale = 1)
{
    static sf::Font font;
    static bool loaded = false;

    if(!loaded)
    {
        font.loadFromFile("VeraMono.ttf");
        loaded = true;
    }

    sf::Text txt;
    txt.setFont(font);
    txt.setString(to_render.c_str());
    txt.setCharacterSize(round(16 * scale));
    //txt.setScale(0.5f, 0.5f);

    pos = round(pos);

    txt.setPosition(pos.x(), pos.y());

    col = col * 255;

    txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), col.w()));

    txt.setOrigin(round(txt.getLocalBounds().width/2.f), 0.f);

    win.draw(txt);
}*/

struct terminal
{
    std::string command;
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

    ///need to wrap text here
    void render_str(sf::RenderWindow& win, const std::string& str, vec2f cpos)
    {
        sf::Text txt;
        txt.setFont(font);

        vec2f pos = cpos;

        for(int i=0; i < str.size(); i++)
        {
            vec2f found_pos = round(pos);

            txt.setString(std::string(1, str[i]));
            txt.setCharacterSize(12);
            txt.setPosition(found_pos.x(), found_pos.y());

            vec4f col = {1,1,1,1};

            col = col * 255.f;

            txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), col.w()));

            vec2i dim = {txt.getGlobalBounds().width, txt.getGlobalBounds().height};

            pos.x() += 8;

            win.draw(txt);
        }
    }

    void render(sf::RenderWindow& win)
    {
        int cheight = 16;

        vec2f start_pos = {4, win.getSize().y - cheight};
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
        }

        /*for(auto& i : key_map)
        {
            if(key.isKeyPressed(i.first))
            {
                term.add_to_command(i.second);
            }
        }*/

        if(ONCE_MACRO(sf::Keyboard::BackSpace))
        {
            term.remove_back();
        }

        if(ONCE_MACRO(sf::Keyboard::Return))
        {
            //term.add_to_command('\n');

            term.command = strip_whitespace(term.command);

            shared.add_back_write(term.command);
            term.text_history.push_back(term.command);
            term.command = "";
        }

        if(shared.has_front_read())
        {
            term.text_history.push_back(shared.get_front_read());
        }

        //std::cout << term.command << std::endl;

        /*std::string next_command;

        std::getline(std::cin, next_command);

        shared.add_back_write(next_command);*/

        term.render(window);

        window.display();
        window.clear();
    }

    return 0;
}
