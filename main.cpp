#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

#include "util.hpp"

#include "colour_interop.hpp"
#include "string_helpers.hpp"
#include <serialise/serialise.hpp>

#include "auto_handlers.hpp"
#include "copy_handler.hpp"
#include "stacktrace.hpp"
#include "editable_string.hpp"
#include "ui_components.hpp"

///ruh roh
///need to structure this project properly
#include "local_commands.hpp"
#include <libncclient/c_shared_data.h>
#include <libncclient/c_net_client.h>
#include <libncclient/nc_util.hpp>
#include <libncclient/c_server_api.h>
#include <libncclient/nc_string_interop.hpp>

bool is_focused(sf::RenderWindow& win)
{
    return win.getSystemHandle() == GetFocus();
}

#define DMAP(A) key_map[sf::Keyboard::A] = tolower((#A)[0]);

//#define HOST_IP "192.168.0.55"
#ifdef EXTERN_IP
#define HOST_IP "77.96.132.101"
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_IP "127.0.0.1"
#endif // LOCAL_IP

#ifdef EXTERN_IP
#define HOST_PORT "6760"
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_PORT "6761"
#endif // LOCAL_IP

///test new repo
int main()
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    stack_on_start();

    c_shared_data shared = sd_alloc();

    if(file_exists("key.key"))
    {
        //shared.auth = read_file_bin("key.key");

        std::string fauth = read_file_bin("key.key");

        sd_set_auth(shared, make_view(fauth));

        std::cout << "loaded auth of " << fauth.size() << std::endl;

        //shared.send_auth = true;
    }

    font.loadFromFile("VeraMono.ttf");

    nc_start(shared, HOST_IP, HOST_PORT);

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

    sf::Clock write_clock;

    sf::Keyboard key;
    sf::Mouse mouse;

    //double diff_s = 0.f;

    bool running = true;

    while(running)
    {
        editable_string* to_edit = &term.command;

        if(chat_win.focused)
            to_edit = &chat_win.command;

        bool enter = false;

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

                if(event.key.code == sf::Keyboard::Home)
                {
                    to_edit->move_cursor(-(int)to_edit->command.size());
                }

                if(event.key.code == sf::Keyboard::End)
                {
                    to_edit->move_cursor(to_edit->command.size());
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

                if(event.key.code == sf::Keyboard::Return)
                {
                    enter = true;
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

        if(enter)
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
                    sd_set_user(shared, make_view(spl[1]));
                }
            }

            if(term.focused)
            {
                if(!sa_is_local_command(term.command.command.c_str()))
                {
                    sized_string current_user = sd_get_user(shared);

                    char* up_handled = sa_default_up_handling(current_user.str, term.command.command.c_str(), "./scripts/");

                    char* server_command = sa_make_generic_server_command(up_handled);

                    std::string str = c_str_to_cpp(server_command);

                    free_string(server_command);
                    free_string(up_handled);
                    free_sized_string(current_user);

                    sd_add_back_write(shared, make_view(str));
                }

                term.auto_handle.clear_internal_state();
            }
            else
            {
                char* chat_command = sa_make_chat_command(chat_win.selected.c_str(), chat_win.command.command.c_str());

                ///TODO
                sd_add_back_write(shared, make_view_from_raw(chat_command));

                free_string(chat_command);
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

            if(term.focused && sa_is_local_command(cmd.c_str()))
            {
                bool should_shutdown = false;

                sized_string found_user = sd_get_user(shared);
                std::string data = handle_local_command(c_str_sized_to_cpp(found_user), cmd, term.auto_handle, should_shutdown, term);
                free_sized_string(found_user);

                term.add_text_from_server(data, chat_win, false);

                if(should_shutdown)
                {
                    running = false;
                }
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

        if(sd_has_front_read(shared))
        {
            sized_string c_data = sd_get_front_read(shared);
            std::string fdata = c_str_sized_to_cpp(c_data);
            free_sized_string(c_data);

            term.add_text_from_server(fdata, chat_win);
        }

        if(client_poll_clock.getElapsedTime().asMilliseconds() > 500)
        {
            sa_do_poll_server(shared);

            client_poll_clock.restart();
        }

        if(write_clock.getElapsedTime().asMilliseconds() > 2000)
        {
            serialise sterm;
            sterm.handle_serialise(term, true);
            sterm.save(terminal_file);

            serialise swindow;
            swindow.handle_serialise(chat_win, true);
            swindow.save(chat_file);

            write_clock.restart();
        }

        ///hmm
        ///this is inadequate
        ///we need to be able to request multiple scripts at once
        ///and receive multiple as well
        if(term.auto_handle.found_unprocessed_autocompletes.size() > 0 && request_clock.getElapsedTime().asMilliseconds() > 300)
        {
            request_clock.restart();

            for(auto& str : term.auto_handle.found_unprocessed_autocompletes)
            {
                sa_do_autocomplete_request(shared, str.c_str());

                break;
            }

            if(term.auto_handle.found_unprocessed_autocompletes.size() > 0)
                term.auto_handle.found_unprocessed_autocompletes.erase(term.auto_handle.found_unprocessed_autocompletes.begin());

            //term.auto_handle.found_unprocessed_autocompletes.clear();
        }

        //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

        term.auto_handle.window_in_focus = is_focused(window);
        chat_win.auto_handle.window_in_focus = is_focused(window);

        term.render(window);
        chat_win.render(window, term.chat_threads);

        term.auto_handle.tab_pressed = ONCE_MACRO(sf::Keyboard::Tab) && is_focused(window);

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

    sd_set_termination(shared);

    sf::sleep(sf::milliseconds(100));

    //while(shared.termination_count != 3) {}

    return 0;
}
