#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

#include "util.hpp"

#include "colour_interop.hpp"
#include "string_helpers.hpp"

#include "auto_handlers.hpp"
#include "copy_handler.hpp"
#include "stacktrace.hpp"
#include "editable_string.hpp"
#include "tokeniser.hpp"
#include "serialisables.hpp"
#include <networking/networking.hpp>

///ruh roh
///need to structure this project properly
#include "local_commands.hpp"
#include <libncclient/c_all.h>
#include <libncclient/nc_util.hpp>
#include <libncclient/nc_string_interop.hpp>
#include <libncclient/c_steam_api.h>

#include <imgui/imgui.h>
#include <imgui-sfml/imgui-SFML.h>

#include "imgui_ui_components.hpp"
#include <nlohmann/json.hpp>
#include <objbase.h>
#include <stdio.h>
#include "font_cfg.hpp"
#include <iomanip>
#include "window_context.hpp"
#include "imguix.hpp"

bool is_focused(bool in_focus)
{
    return in_focus;
}

std::string make_lower(std::string in)
{
    for(auto& i : in)
    {
        if(isalpha(i))
            i = tolower(i);
    }

    return in;
}

#define DMAP(A) key_map[sf::Keyboard::A] = make_lower((#A));
#define SMAP(A, S) key_map[sf::Keyboard::A] = ((#S));
#define MMAP(A, S) mouse_map[sf::Mouse::A] = ((#S));

#define SPECIAL_MAP(A, S) on_input_map[sf::Keyboard::A] = ((#S));

//#define HOST_IP "192.168.0.55"
#ifdef EXTERN_IP
//#define HOST_IP "77.96.132.101"
//#define HOST_IP "netcodegame.com"
#define HOST_IP "77.97.17.179"
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_IP "127.0.0.1"
#endif // LOCAL_IP

#ifdef EXTERN_IP
#define HOST_PORT 6760
#define HOST_PORT_SSL 6780
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_PORT 6761
#define HOST_PORT_SSL 6781
#endif // LOCAL_IP

std::string default_up_handling(const std::string& user, const std::string& server_msg, const std::string& scripts_dir)
{
    std::string up = "#up ";
    std::string up_es6 = "#up_es6 ";
    std::string dry = "#dry ";

    std::vector<std::string> strings = no_ss_split(server_msg, " ");

    if((starts_with(server_msg, up) || starts_with(server_msg, dry)) && strings.size() == 2)
    {
        std::string name = strings[1];

        std::string diskname = scripts_dir + user + "." + name + ".es5.js";
        std::string diskname_es6 = scripts_dir + user + "." + name + ".js";
        std::string diskname_ts = scripts_dir + user + "." + name + ".ts";

        std::string comm = up;

        if(starts_with(server_msg, dry))
            comm = dry;

        std::string data = "";

        if(file_exists(diskname))
            data = read_file(diskname);

        if(file_exists(diskname_es6))
        {
            data = read_file(diskname_es6);
            comm = up_es6;
        }

        if(file_exists(diskname_ts))
        {
            data = read_file(diskname_ts);
            comm = up_es6;
        }

        return comm + name + " " + data;
    }

    return server_msg;
}

void handle_auth(c_steam_api csapi, connection& conn)
{
    if(steam_api_enabled(csapi))
    {
        ///embed key.key
        if(file_exists("key.key"))
        {
            printf("Embedding key auth in steam auth\n");

            steam_api_request_encrypted_token(csapi, make_view(read_file_bin("key.key")));
        }
        else
        {
            printf("Steam auth, no key auth");

            steam_api_request_encrypted_token(csapi, make_view_from_raw(""));
        }

        while(steam_api_should_wait_for_encrypted_token(csapi)){}

        if(!steam_api_has_encrypted_token(csapi))
        {
            printf("Failed to get encrypted token");
            throw std::runtime_error("Could not fetch steam token");
        }

        sized_string str = steam_api_get_encrypted_token(csapi);

        std::string etoken = c_str_consume(str);
        etoken = binary_to_hex(etoken);

        nlohmann::json data;
        data["type"] = "steam_auth";
        data["data"] = etoken;

        conn.write(data.dump());
    }
    ///use key based auth
    else if(file_exists("key.key"))
    {
        printf("Pure key auth\n");

        nlohmann::json data;
        data["type"] = "key_auth";
        data["data"] = binary_to_hex(read_file_bin("key.key"));

        conn.write(data.dump());
    }
    else
    {
        ///no auth available
        printf("No auth methods available, use steam or key.key file");
        throw std::runtime_error("No auth method available");
    }
}

///test new repo
int main()
{
    //test();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    stack_on_start();

    //token_tests();

    c_steam_api csapi = steam_api_alloc();

    connection conn;
    conn.connect(HOST_IP, HOST_PORT_SSL, connection_type::SSL);

    handle_auth(csapi, conn);

    ///need to write auth data here!

    window_context window_ctx;

    sf::RenderWindow& window = window_ctx.win;

    ImGui::SFML::Init(window, false);

    ImGui::SetStyleLinearColor(window_ctx.is_srgb);

    ImGui::PushSrgbStyleColor(ImGuiCol_WindowBg, ImGuiX::GetBgCol());

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 0;
    style.WindowRounding = 0;
    style.ChildRounding = 0;
    style.ChildBorderSize = 0;
    style.FrameBorderSize = 0;
    //style.PopupBorderSize = 0;
    style.WindowBorderSize = 1;

    /*style.FramePadding = ImVec2(0,0);
    style.DisplaySafeAreaPadding = ImVec2(0,0);
    style.DisplayWindowPadding = ImVec2(0,0);
    style.ItemInnerSpacing = ImVec2(0,0);*/

    /*ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", 13.f);
    io.Fonts->AddFontDefault();*/

    font_selector font_select;
    font_select.reset_default_fonts();

    terminal_imgui term;
    chat_window chat_win;

    bool should_coordinate_focus = true;

    sf::Event event;

    std::map<sf::Keyboard::Key, std::string> key_map;
    std::map<sf::Keyboard::Key, std::string> on_input_map;
    std::map<sf::Mouse::Button, std::string> mouse_map;

    DMAP(A);DMAP(B);DMAP(C);
    DMAP(D);DMAP(E);DMAP(F);
    DMAP(G);DMAP(H);DMAP(I);
    DMAP(J);DMAP(K);DMAP(L);
    DMAP(M);DMAP(N);DMAP(O);
    DMAP(P);DMAP(Q);DMAP(R);
    DMAP(S);DMAP(T);DMAP(U);
    DMAP(V);DMAP(W);DMAP(X);
    DMAP(Y);DMAP(Z);

    SMAP(Return, return);
    SMAP(BackSpace, backspace);
    SMAP(Delete, delete);

    SMAP(Insert,insert);
    SMAP(Tab, tab);
    SMAP(Delete, delete);

    SPECIAL_MAP(Return, return);
    SPECIAL_MAP(BackSpace, backspace);
    SPECIAL_MAP(Delete, delete);

    SPECIAL_MAP(Insert,insert);
    SPECIAL_MAP(Tab, tab);
    SPECIAL_MAP(Delete, delete);

    SMAP(Up, up);
    SMAP(Down, down);
    SMAP(Right, right);
    SMAP(Left, left);
    SMAP(Home, home);
    SMAP(End, end);
    SMAP(PageUp, pageup);
    SMAP(PageDown, pagedown);

    SPECIAL_MAP(Up, up);
    SPECIAL_MAP(Down, down);
    SPECIAL_MAP(Right, right);
    SPECIAL_MAP(Left, left);
    SPECIAL_MAP(Home, home);
    SPECIAL_MAP(End, end);
    SPECIAL_MAP(PageUp, pageup);
    SPECIAL_MAP(PageDown, pagedown);

    SMAP(Num1, 1);
    SMAP(Num2, 2);
    SMAP(Num3, 3);
    SMAP(Num4, 4);
    SMAP(Num5, 5);
    SMAP(Num6, 6);
    SMAP(Num7, 7);
    SMAP(Num8, 8);
    SMAP(Num9, 9);
    SMAP(Num0, 0);

    SMAP(LShift, lshift);
    SMAP(RShift, rshift);
    SMAP(LControl, lctrl);
    SMAP(RControl, rctrl);
    SMAP(LAlt, lalt);
    SMAP(RAlt, ralt);

    SPECIAL_MAP(LShift, lshift);
    SPECIAL_MAP(RShift, rshift);
    SPECIAL_MAP(LControl, lctrl);
    SPECIAL_MAP(RControl, rctrl);
    SPECIAL_MAP(LAlt, lalt);
    SPECIAL_MAP(RAlt, ralt);

    SMAP(LBracket, [);
    SMAP(RBracket, ]);

    key_map[sf::Keyboard::SemiColon] = ";";
    key_map[sf::Keyboard::Comma] = ",";
    SMAP(Period, .);
    SMAP(Tilde, ~);
    SMAP(Equal, =);
    SMAP(Hyphen, -);

    SMAP(Space, space);

    SMAP(Escape, escape);
    SPECIAL_MAP(Escape, escape);

    key_map[sf::Keyboard::Quote] = "\'";
    SMAP(Slash, /);
    SMAP(Backslash, \\);

    MMAP(Left, lmouse);
    MMAP(Right, rmouse);
    MMAP(Middle, mmouse);

    std::string terminal_file = "./terminal_v5.txt";
    std::string chat_file = "./chat_v5.txt";
    std::string settings_file = "./text_sett.txt";
    std::string font_file = "./font_sett.txt";

    if(file_exists(terminal_file))
    {
        nlohmann::json dat = load_from_file_json(terminal_file);
        deserialise(dat, term, serialise_mode::DISK);
    }

    if(file_exists(chat_file))
    {
        nlohmann::json dat = load_from_file_json(chat_file);
        deserialise(dat, chat_win, serialise_mode::DISK);
    }

    if(file_exists(font_file))
    {
        nlohmann::json dat = load_from_file_json(font_file);
        deserialise(dat, font_select, serialise_mode::DISK);
    }

    sf::Clock render_clock;

    sf::Clock request_clock;

    sf::Clock write_clock;

    int active_frames_restart = 1;
    int active_frames = active_frames_restart;

    sf::Keyboard key;
    sf::Mouse mouse;

    sf::Clock imgui_delta;

    sf::Clock mouse_clock;
    float mouse_send_time_ms = 33;

    #ifdef TESTING
    std::vector<std::string> api_calls;
    #endif // TESTING

    //double diff_s = 0.f;

    editable_string realtime_shim;
    std::vector<std::string> realtime_str;
    std::vector<std::string> on_pressed;
    std::vector<std::string> on_released;

    bool running = true;
    bool focused = true;

    double script_mousewheel_delta = 0.;

    term.invalidate();

    font_render_context font_context(font_select, window_ctx);

    std::string current_user = "";

    sf::Clock connection_clock;

    while(running)
    {
        if(font_select.update_rebuild(window, font_select.current_base_font_size))
        {
            term.invalidate();

            active_frames = active_frames_restart;
        }

        if(connection_clock.getElapsedTime().asSeconds() > 5 && !conn.client_connected_to_server)
        {
            conn.connect(HOST_IP, HOST_PORT_SSL, connection_type::SSL);
            connection_clock.restart();

            handle_auth(csapi, conn);

            term.add_text("Connecting...");
        }

        realtime_shim.clear_command();
        realtime_str.clear();
        on_pressed.clear();
        on_released.clear();

        editable_string no_string;
        editable_string* to_edit = &no_string;

        editable_string* hovered_string = &no_string;

        if(term.focused)
            to_edit = &term.command;

        if(chat_win.focused)
            to_edit = &chat_win.command;

        if(term.get_id_of_focused_realtime_window() != -1)
            to_edit = &realtime_shim;

        if(term.hovered)
            hovered_string = &term.command;

        if(chat_win.hovered)
            hovered_string = &chat_win.command;

        if(term.get_id_of_focused_realtime_window() != -1 && term.realtime_script_windows[term.get_id_of_focused_realtime_window()].hovered)
            hovered_string = &realtime_shim;

        bool enter = false;

        float mouse_delta = 0.f;

        bool skip_first_event = false;

        if(steam_api_overlay_is_open(csapi))
        {
            active_frames = active_frames_restart;
        }

        ///doesn't work properly for some reason
        /*if(steam_api_overlay_needs_present(csapi))
        {
            active_frames = active_frames_restart;

            printf("Reset present\n");
        }*/

        steam_api_pump_events(csapi);

        if(active_frames <= 0)
        {
            int loops = 5;

            if(!steam_api_enabled(csapi))
            {
                loops = 40;
            }

            for(int i=0; i < loops; i++)
            {
                if(window.pollEvent(event))
                {
                    skip_first_event = true;
                    break;
                }

                if(conn.has_read())
                    break;

                steam_api_pump_events(csapi);

                if(steam_api_overlay_is_open(csapi))
                {
                    active_frames = active_frames_restart;

                    break;
                }

                sf::sleep(sf::milliseconds(4));
            }
        }
        else
        {
            active_frames--;
        }

        while(skip_first_event || window.pollEvent(event))
        {
            skip_first_event = false;
            ImGui::SFML::ProcessEvent(event);

            active_frames = active_frames_restart;

            if(event.type == sf::Event::GainedFocus)
            {
                focused = true;

                term.invalidate();
            }

            if(event.type == sf::Event::LostFocus)
                focused = false;

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

                        term.last_line_invalidate();

                        std::string str = std::string(1, event.text.unicode);

                        if(str == " ")
                            str = "space";

                        realtime_str.push_back(str);
                    }
                }
            }

            if(event.type == sf::Event::Resized)
            {
                window.setSize({event.size.width, event.size.height});
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));

                term.invalidate();

                window_ctx.width = event.size.width;
                window_ctx.height = event.size.height;

                window_ctx.save();
            }

            if(event.type == sf::Event::MouseButtonReleased)
            {
                if(mouse_map.find(event.mouseButton.button) != mouse_map.end())
                    on_pressed.push_back(mouse_map[event.mouseButton.button]);
            }

            if(event.type == sf::Event::MouseButtonPressed)
            {
                if(mouse_map.find(event.mouseButton.button) != mouse_map.end())
                    on_released.push_back(mouse_map[event.mouseButton.button]);

                if(mouse_map.find(event.mouseButton.button) != mouse_map.end())
                    realtime_str.push_back(mouse_map[event.mouseButton.button]);

                if(event.mouseButton.button == sf::Mouse::Right)
                {
                    std::string add_text = get_clipboard_contents();

                    for(auto& i : add_text)
                    {
                        to_edit->add_to_command(i);
                    }

                    term.last_line_invalidate();
                }
            }

            if(event.type == sf::Event::KeyReleased)
            {
                if(key_map.find(event.key.code) != key_map.end())
                    on_released.push_back(key_map[event.key.code]);
            }

            if(event.type == sf::Event::KeyPressed)
            {
                if(on_input_map.find(event.key.code) != on_input_map.end())
                    realtime_str.push_back(on_input_map[event.key.code]);

                if(key_map.find(event.key.code) != key_map.end())
                    on_pressed.push_back(key_map[event.key.code]);

                term.last_line_invalidate();

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
                    if(!key.isKeyPressed(sf::Keyboard::LControl))
                        to_edit->move_cursor(-1);
                    else
                        to_edit->move_cursor(-5);
                }

                if(event.key.code == sf::Keyboard::Right)
                {
                    if(!key.isKeyPressed(sf::Keyboard::LControl))
                        to_edit->move_cursor(1);
                    else
                        to_edit->move_cursor(5);
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
                }

                if(event.key.code == sf::Keyboard::V)
                {
                    if(key.isKeyPressed(sf::Keyboard::LControl) && is_focused(focused))
                    {
                        std::string add_text = get_clipboard_contents();

                        for(auto& i : add_text)
                        {
                            to_edit->add_to_command(i);
                        }
                    }
                }

                if(event.key.code == sf::Keyboard::F1)
                {
                    font_select.is_open = !font_select.is_open;
                }

                if(event.key.code == sf::Keyboard::Return)
                {
                    if(to_edit != &realtime_shim && to_edit != &no_string)
                    {
                        if(!key.isKeyPressed(sf::Keyboard::LControl) && !key.isKeyPressed(sf::Keyboard::LShift))
                            enter = true;
                        else
                            to_edit->add_to_command('\n');
                    }
                    else
                        to_edit->add_to_command('\n');
                }

                ///TODO: DONT FORGET ABOUT FIXING TERMINAL INVALIDATION FOR THESE
                if(event.key.code == sf::Keyboard::PageDown)
                {
                    if(hovered_string == &term.command)
                    {
                        mouse_delta -= term.render_height - 2;

                        if(term.scroll_hack.scrolled + mouse_delta < 0)
                            mouse_delta = -term.scroll_hack.scrolled;

                    }

                    if(hovered_string == &chat_win.command)
                    {
                        mouse_delta -= chat_win.render_height - 2;

                        if(chat_win.scroll_hack.scrolled + mouse_delta < 0)
                            mouse_delta = -chat_win.scroll_hack.scrolled;
                    }

                    term.last_line_invalidate();
                }

                if(event.key.code == sf::Keyboard::PageUp)
                {
                    if(hovered_string == &term.command)
                        mouse_delta += term.render_height - 2;
                    if(hovered_string == &chat_win.command)
                        mouse_delta += chat_win.render_height - 2;

                    term.last_line_invalidate();
                }
            }

            if(event.type == sf::Event::MouseButtonPressed)
            {
                if(event.mouseButton.button == sf::Mouse::Left)
                {
                    term.last_line_invalidate();

                    get_global_copy_handler()->on_lclick({event.mouseButton.x, event.mouseButton.y});
                }
            }

            if(event.type == sf::Event::MouseButtonReleased)
            {
                if(event.mouseButton.button == sf::Mouse::Left)
                {
                    term.last_line_invalidate();

                    get_global_copy_handler()->on_lclick_release({event.mouseButton.x, event.mouseButton.y});
                }
            }

            if(event.type == sf::Event::MouseWheelScrolled)
            {
                //term.invalidate();

                mouse_delta += event.mouseWheelScroll.delta;
                script_mousewheel_delta += event.mouseWheelScroll.delta;
            }
        }

        if(term.get_id_of_focused_realtime_window() != -1 && (realtime_str.size() > 0 || on_pressed.size() > 0 || on_released.size() > 0))
        {
            nlohmann::json data;
            data["type"] = "send_keystrokes_to_script";
            data["id"] = term.get_id_of_focused_realtime_window();
            data["input_keys"] = realtime_str;
            data["pressed_keys"] = on_pressed;
            data["released_keys"] = on_released;

            conn.write(data.dump());
        }

        if(term.get_id_of_focused_realtime_window() != -1 && mouse_clock.getElapsedTime().asMicroseconds() / 1000. >= mouse_send_time_ms && is_focused(focused))
        {
            mouse_clock.restart();

            sf::Mouse mouse;

            auto vmouse = mouse.getPosition(window);

            vec2f mpos = {vmouse.x, vmouse.y};

            realtime_script_run& run = term.realtime_script_windows[term.get_id_of_focused_realtime_window()];

            mpos = mpos - run.current_tl_cursor_pos;

            vec2f br_absolute = run.current_pos + (vec2f){run.current_dim.x(), run.current_dim.y()};
            vec2f relative_dim = br_absolute - run.current_tl_cursor_pos;

            //vec2i dim = run.current_dim;

            vec2f dim = relative_dim;

            if(mpos.x() >= 0 && mpos.y() >= 0 && mpos.x() <= dim.x() && mpos.y() <= dim.y())
            {
                vec2f char_mpos = mpos / (vec2f){char_inf::cwidth, char_inf::cheight};

                nlohmann::json data;
                data["type"] = "update_mouse_to_script";
                data["id"] = term.get_id_of_focused_realtime_window();
                data["mouse_x"] = char_mpos.x();
                data["mouse_y"] = char_mpos.y();
                data["mousewheel_x"] = 0.f;
                data["mousewheel_y"] = script_mousewheel_delta;

                conn.write(data.dump());
            }

            script_mousewheel_delta = 0;
        }

        if(!is_focused(focused) || term.get_id_of_focused_realtime_window() == -1)
            script_mousewheel_delta = 0;

        term.scroll_hack.scrolled_this_frame = mouse_delta;
        chat_win.scroll_hack.scrolled_this_frame = mouse_delta;

        for(auto& i : term.realtime_script_windows)
        {
            i.second.scroll_hack.scrolled_this_frame = mouse_delta;
        }

        ImGui::SFML::Update(window,  imgui_delta.restart());

        ImGui::PushFont(font_select.get_base_font());

        //ImGui::ShowDemoWindow(nullptr);
        //ImGui::ShowStyleEditor(nullptr);

        font_select.render(window_ctx);

        if(window_ctx.srgb_dirty)
        {
            ImGui::SetStyleLinearColor(window_ctx.is_srgb);

            active_frames = active_frames_restart;
        }

        term.check_insert_user_command();

        if(starts_with(to_edit->command, "user user ") && term.focused)
        {
            if((int)to_edit->command.size() > (int)strlen("user "))
            {
                for(int i=0; i < (int)strlen("user "); i++)
                {
                    to_edit->command.erase(to_edit->command.begin());
                    to_edit->cursor_pos_idx = to_edit->command.size();
                }
            }

            active_frames = active_frames_restart;
        }

        if(enter && to_edit->command.size() > 0)
        {
            term.invalidate();

            //term.add_to_command('\n');

            if(term.focused)
            {
                term.consider_resetting_scrollbar = true;
            }

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
                    current_user = spl[1];
                }
            }

            if(term.focused)
            {
                if(!sa_is_local_command(make_view(term.command.command)))
                {
                    std::string up_data = default_up_handling(current_user, term.command.command, "./scripts/");

                    nlohmann::json data;
                    data["type"] = "generic_server_command";
                    data["data"] = up_data;

                    conn.write(data.dump());
                }
            }
            else
            {
                //if(chat_win.command.command.size() > 0 && chat_win.command.command.back() == '\n')
                //    chat_win.command.command.pop_back();

                std::string command = chat_win.command.command;

                bool bump = false;

                if(command == "/join")
                {
                    term.add_text("Syntax is /join channel password");
                    bump = true;
                }
                else if(command == "/leave")
                {
                    term.add_text("Syntax is /leave channel");
                    bump = true;
                }
                else if(command == "/create")
                {
                    term.add_text("Syntax is /create channel password");
                    bump = true;
                }
                else if(starts_with(command, "/"))
                {
                    bump = true;

                    int idx = 0;

                    for(; idx < (int)command.size() && command[idx] != ' '; idx++);

                    if(idx + 1 >= (int)command.size())
                    {
                        term.add_text("First argument must be a channel name, eg /join global");
                    }
                    else
                    {
                        idx++;

                        std::string channel_name;

                        for(; idx < (int)command.size() && command[idx] != ' '; idx++)
                        {
                            channel_name.push_back(command[idx]);
                        }

                        std::string channel_password;

                        if(idx + 1 < (int)command.size())
                        {
                            idx++;

                            ///password may include whitespace
                            for(; idx < (int)command.size(); idx++)
                            {
                                channel_password.push_back(command[idx]);
                            }
                        }

                        std::string args = "{name:\"" + escape_str(channel_name) + "\"";

                        if(channel_password != "")
                        {
                            args += ", password:\"" + escape_str(channel_password) + "\"}";
                        }
                        else
                        {
                            args += "}";
                        }

                        std::string final_command;

                        if(starts_with(command, "/join"))
                        {
                            final_command = "#channel.join(" + args + ")";
                        }
                        else if(starts_with(command, "/leave"))
                        {
                            final_command = "#channel.leave(" + args + ")";
                        }
                        else if(starts_with(command, "/create"))
                        {
                            final_command = "#channel.create(" + args + ")";
                        }
                        else
                        {
                            term.add_text("Not a valid command, try /join, /leave or /create");
                        }

                        if(final_command != "")
                        {
                            nlohmann::json data;
                            data["type"] = "client_chat";
                            data["respond"] = 1;
                            data["data"] = final_command;

                            conn.write(data.dump());
                        }
                    }
                }
                else
                {
                    std::string escaped_string = escape_str(chat_win.command.command);

                    nlohmann::json data;
                    data["type"] = "client_chat";
                    data["data"] = "#hs.msg.send({channel:\"" + chat_win.selected + "\", msg:\"" + escaped_string + "\"})";

                    conn.write(data.dump());
                }

                if(bump)
                {
                    term.add_text_to_current_chat_thread(chat_win, command);
                    term.invalidate();
                }
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

            if(term.focused && sa_is_local_command(make_view(cmd)))
            {
                bool should_shutdown = false;

                std::string data = handle_local_command(current_user, cmd, term.auto_handle, should_shutdown, term);

                term.add_text_from_server(current_user, data, chat_win, false);

                if(should_shutdown)
                {
                    running = false;
                }
            }

            nlohmann::json d1 = serialise(term, serialise_mode::DISK);
            nlohmann::json d2 = serialise(chat_win, serialise_mode::DISK);

            save_to_file_json(terminal_file, d1);
            save_to_file_json(chat_file, d2);
        }
        else if(enter && to_edit->command.size() == 0)
        {
            term.add_text(" ");
        }

        chat_win.tick();

        auto sf_mpos = mouse.getPosition(window);
        vec2f vpos = {sf_mpos.x, sf_mpos.y};

        if(mouse.isButtonPressed(sf::Mouse::Left) && is_focused(focused))
        {
            active_frames = active_frames_restart;

            get_global_copy_handler()->on_hold_lclick(window,  vpos);
        }

        while(conn.has_read())
        {
            active_frames = active_frames_restart;

            write_data dat = conn.read_from();

            std::string fdata = dat.data;

            conn.pop_read(dat.id);

            #ifdef TESTING
            api_calls.push_back(fdata);
            #endif // TESTING

            ///this is temporary before the other end of the api gets changed
            std::string auth_str = "command_auth secret ";

            if(fdata.size() >= auth_str.size() && fdata.substr(0, auth_str.size()) == auth_str)
            {
                auto start = fdata.begin() + auth_str.size();
                std::string key_file = "key.key";
                std::string key(start, fdata.end());

                if(!file_exists(key_file))
                {
                    write_all_bin(key_file, key);

                    term.add_text(make_success_col("Success! Try user lowercase_name to get started, and then #scripts.core()"));
                }
                else
                {
                    term.add_text(make_error_col("Did not overwrite existing key file, you are already registered"));
                }
            }

            term.add_text_from_server(current_user, fdata, chat_win);
        }

        if(write_clock.getElapsedTime().asMilliseconds() > 2000)
        {
            save_to_file_json(terminal_file, serialise(term, serialise_mode::DISK));
            save_to_file_json(chat_file, serialise(chat_win, serialise_mode::DISK));

            write_clock.restart();
        }

        ///hmm
        ///this is inadequate
        ///we need to be able to request multiple scripts at once
        ///and receive multiple as well
        if(term.auto_handle.found_unprocessed_autocompletes.size() > 0 && request_clock.getElapsedTime().asMilliseconds() > 300)
        {
            request_clock.restart();

            for(const std::string& str : term.auto_handle.found_unprocessed_autocompletes)
            {
                nlohmann::json data;
                data["type"] = "autocomplete_request";
                data["data"] = str;

                conn.write(data.dump());

                break;
            }

            if(term.auto_handle.found_unprocessed_autocompletes.size() > 0)
                term.auto_handle.found_unprocessed_autocompletes.erase(term.auto_handle.found_unprocessed_autocompletes.begin());

            //term.auto_handle.found_unprocessed_autocompletes.clear();
        }

        if((term.focused || term.get_id_of_focused_realtime_window() != 1) && is_focused(focused) && key.isKeyPressed(sf::Keyboard::LControl) && ONCE_MACRO(sf::Keyboard::C))
        {
            nlohmann::json data;
            data["type"] = "client_terminate_scripts";
            data["id"] = -1;

            conn.write(data.dump());
        }

        //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

        term.auto_handle.window_in_focus = is_focused(focused);
        chat_win.auto_handle.window_in_focus = is_focused(focused);

        //test_imgui_term.text_history = term.text_history;

        int was_closed_id = -1;

        //test_imgui_term.render(window);
        term.render_realtime_windows(conn, was_closed_id);
        chat_win.render(window, term.chat_threads, should_coordinate_focus);
        term.render(window, should_coordinate_focus);

        should_coordinate_focus = false;

        if(was_closed_id != -1)
        {
            nlohmann::json data;
            data["type"] = "client_terminate_scripts";
            data["id"] = was_closed_id;

            conn.write(data.dump());
        }

        if(term.auto_handle.tab_pressed)
        {
            active_frames = active_frames_restart;
            term.last_line_invalidate();
        }

        term.auto_handle.tab_pressed = ONCE_MACRO(sf::Keyboard::Tab) && is_focused(focused);

        if(term.auto_handle.tab_pressed)
        {
            term.last_line_invalidate();
        }

        ///this is a hack to fix the fact that sometimes
        ///click input doesn't make clean click/release pairs
        if(!mouse.isButtonPressed(sf::Mouse::Left))
        {
            get_global_copy_handler()->finished = false;
            get_global_copy_handler()->held = false;
        }

        ImGui::PopFont();

        ImGui::SFML::Render(window);

        window.display();
        //window.clear(sf::Color(bg_col.x(), bg_col.y(), bg_col.z()));
        window.clear();

        sf::sleep(sf::milliseconds(4));

        int lcwidth = char_inf::cwidth;
        int lcheight = char_inf::cheight;

        char_inf::cwidth = ImGui::CalcTextSize("A").x + char_inf::extra_glyph_spacing;
        char_inf::cheight = ImGui::CalcTextSize("A").y;

        if(char_inf::cwidth != lcwidth || char_inf::cheight != lcheight)
        {
            active_frames = active_frames_restart;
            term.invalidate();
        }
    }

    save_to_file_json(terminal_file, serialise(term, serialise_mode::DISK));
    save_to_file_json(chat_file, serialise(chat_win, serialise_mode::DISK));

    CoUninitialize();

    return 0;
}
