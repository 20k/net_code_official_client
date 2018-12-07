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
#include "tokeniser.hpp"

///ruh roh
///need to structure this project properly
#include "local_commands.hpp"
#include <libncclient/c_all.h>
#include <libncclient/nc_util.hpp>
#include <libncclient/nc_string_interop.hpp>

#include <imgui/imgui.h>
#include <imgui-sfml/imgui-SFML.h>

#include "imgui_ui_components.hpp"
#include <json/json.hpp>
#include <objbase.h>
#include <stdio.h>
#include "text_editor.hpp"
#include "font_cfg.hpp"
#include <iomanip>
#include "window_context.hpp"
#include "steamapi.hpp"

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
#define HOST_PORT "6760"
#define HOST_PORT_SSL "6780"
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_PORT "6761"
#define HOST_PORT_SSL "6781"
#endif // LOCAL_IP

#ifdef FONT_TEST
void test()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "hi");

    ImGui::SFML::Init(window, false);

    ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    ImFont* font = io.Fonts->AddFontFromFileTTF("VeraMono.ttf", 14.f);

    ImGuiFreeType::BuildFontAtlas(io.Fonts, 0);

    assert(font != nullptr);
    update_font_texture_safe();

    //assert(font->ConfigData.size() != 0);

    font_selector font_select;
    font_select.WantRebuild = false;

    sf::Clock imgui_delta;

    while(1)
    {
        if(font_select.UpdateRebuild())
        {
            update_font_texture_safe();
        }

        sf::Event event;

        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
        }

        ImGui::SFML::Update(window,  imgui_delta.restart());

        ImGui::Begin("Hi");

        ImGui::Text("Test");

        ImGui::End();

        font_select.ShowFreetypeOptionsWindow();

        ImGui::SFML::Render(window);
        window.display();
        window.clear();
    }
}
#endif // FONT_TEST

///test new repo
int main()
{
    //test();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    stack_on_start();

    //token_tests();

    c_shared_data shared = sd_alloc();

    //font.loadFromFile("VeraMono.ttf");

    nc_start_ssl(shared, HOST_IP, HOST_PORT_SSL);

    /*int window_width = 1200;
    int window_height = 600;

    std::string general_file = "./window.txt";

    if(file_exists(general_file))
    {
        using nlohmann::json;

        std::string str = read_file_bin(general_file);

        try
        {
            json j;
            j = json::parse(str);

            if(j.find("width_px") != j.end() && j.find("height_px") != j.end())
            {
                window_width = j["width_px"];
                window_height = j["height_px"];
                //window.setSize(sf::Vector2u((int)j["width_px"], (int)j["height_px"]));
            }
        }
        catch(...){}
    }

    sf::ContextSettings sett;
    sett.antialiasingLevel = 1;
    sett.sRgbCapable = use_srgb;

    sf::RenderWindow window;
    window.create(sf::VideoMode(window_width, window_height), "net_code", sf::Style::Default, sett);
    window.resetGLStates();*/

    window_context window_ctx;

    sf::RenderWindow& window = window_ctx.win;

    ImGui::SFML::Init(window, false);

    //ImGuiFreeType::BuildFontAtlas(io.Fonts, 0);
    //ImGui::SFML::UpdateFontTexture();

    //My name is

    //ImVec4 vec = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive);
    //ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0,0,0,0));
    //printf("%f %f %f %f\n", vec.x, vec.y, vec.z, vec.w);

    ImGui::SetStyleSrgb(window_ctx.is_srgb);

    vec3f bg_col = {30, 30, 30};

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(bg_col.x()/255.f, bg_col.y()/255.f, bg_col.z()/255.f, 255/255.f));

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 0;
    style.WindowRounding = 0;
    style.ChildRounding = 0;
    /*style.ChildBorderSize = 0;
    style.FrameBorderSize = 0;
    //style.PopupBorderSize = 0;
    style.WindowBorderSize = 0;*/

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

    text_editor_manager text_editor(font_select);
    text_editor.set_is_srgb(window_ctx.is_srgb);

    std::string terminal_file = "./terminal_v5.txt";
    std::string chat_file = "./chat_v5.txt";
    std::string settings_file = "./text_sett.txt";
    std::string font_file = "./font_sett.txt";

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

    if(file_exists(font_file))
    {
        serialise sfont;
        sfont.load(font_file);
        sfont.handle_serialise(font_select, false);
    }

    text_editor.load();

    sf::Clock render_clock;

    sf::Clock client_poll_clock;

    sf::Clock request_clock;

    sf::Clock write_clock;

    sf::Clock inactivity_timer;
    double inactivity_time_ms = 100;

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

    while(running)
    {
        if(text_editor.dirty_font)
        {
            font_select.reset_default_fonts(text_editor.current_font_size);

            text_editor.dirty_font = false;
        }

        if(font_select.update_rebuild(window, text_editor.current_font_size))
        {
            term.invalidate();

            inactivity_timer.restart();
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

        if(inactivity_timer.getElapsedTime().asMilliseconds() > inactivity_time_ms)
        {
            for(int i=0; i < 100; i++)
            {
                sf::sleep(sf::milliseconds(4));

                if(window.pollEvent(event))
                {
                    skip_first_event = true;
                    break;
                }

                if(sd_has_front_read(shared))
                    break;
            }
        }

        while(skip_first_event || window.pollEvent(event))
        {
            skip_first_event = false;
            ImGui::SFML::ProcessEvent(event);

            inactivity_timer.restart();

            if(event.type == sf::Event::GainedFocus)
            {
                text_editor.on_focus_window();
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
                if(text_editor.is_open && text_editor.is_focused)
                {
                    if(event.key.code == sf::Keyboard::Z && key.isKeyPressed(sf::Keyboard::LControl))
                    {
                        text_editor.should_undo = true;
                    }

                    if(event.key.code == sf::Keyboard::Y && key.isKeyPressed(sf::Keyboard::LControl))
                    {
                        text_editor.should_redo = true;
                    }
                }

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

                if(event.key.code == sf::Keyboard::F2)
                {
                    text_editor.is_open = !text_editor.is_open;
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
            sized_view* view = new sized_view[realtime_str.size()];
            sized_view* pressed_view = new sized_view[on_pressed.size()];
            sized_view* released_view = new sized_view[on_released.size()];

            for(int i=0; i < (int)realtime_str.size(); i++)
            {
                view[i] = make_view(realtime_str[i]);

                //std::cout << realtime_str[i] << std::endl;
            }

            for(int i=0; i < (int)on_pressed.size(); i++)
            {
                pressed_view[i] = make_view(on_pressed[i]);

                //std::cout << "pressed " << on_pressed[i] << std::endl;
            }

            for(int i=0; i < (int)on_released.size(); i++)
            {
                released_view[i] = make_view(on_released[i]);
            }

            ///pipe keys to server
            ///todo make enter work
            sa_do_send_keystrokes_to_script(shared, term.get_id_of_focused_realtime_window(),
                                            view, realtime_str.size(),
                                            pressed_view, on_pressed.size(),
                                            released_view, on_released.size());

            delete [] view;
            delete [] pressed_view;
            delete [] released_view;
            realtime_str.clear();
            on_pressed.clear();
            on_released.clear();
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

                sa_do_update_mouse_to_script(shared, term.get_id_of_focused_realtime_window(), 0.f, script_mousewheel_delta, char_mpos.x(), char_mpos.y());
            }

            script_mousewheel_delta = 0;
        }

        if(!is_focused(focused) || term.get_id_of_focused_realtime_window() == -1)
            script_mousewheel_delta = 0;

        //printf("delta %f\n", mouse_delta);

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
            text_editor.set_is_srgb(window_ctx.is_srgb);
            ImGui::SetStyleSrgb(window_ctx.is_srgb);

            inactivity_timer.restart();
        }

        text_editor.tick();
        text_editor.render(shared);
        text_editor.check_for_external_modifications();

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

            inactivity_timer.restart();
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
                    sd_set_user(shared, make_view(spl[1]));
                    text_editor.set_current_user(spl[1]);
                }
            }

            if(term.focused)
            {
                if(!sa_is_local_command(make_view(term.command.command)))
                {
                    sized_string current_user = sd_get_user(shared);

                    sized_string up_handled = sa_default_up_handling(make_view(current_user), make_view(term.command.command), make_view_from_raw("./scripts/"));

                    sized_string server_command = sa_make_generic_server_command(make_view(up_handled));

                    std::string str = c_str_sized_to_cpp(server_command);

                    free_sized_string(server_command);
                    free_sized_string(up_handled);
                    free_sized_string(current_user);

                    sd_add_back_write(shared, make_view(str));
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
                    sd_add_back_read(shared, make_view("command Syntax is /join channel password"));
                    bump = true;
                }
                else if(command == "/leave")
                {
                    sd_add_back_read(shared, make_view("command Syntax is /leave channel"));
                    bump = true;
                }
                else if(command == "/create")
                {
                    sd_add_back_read(shared, make_view("command Syntax is /create channel password"));
                    bump = true;
                }
                else if(starts_with(command, "/"))
                {
                    bump = true;

                    int idx = 0;

                    for(; idx < (int)command.size() && command[idx] != ' '; idx++);

                    if(idx + 1 >= (int)command.size())
                    {
                        sd_add_back_read(shared, make_view("command First argument must be a channel name, eg /join global"));
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
                            sd_add_back_read(shared, make_view("command Not a valid command, try /join, /leave or /create"));
                        }

                        if(final_command != "")
                        {
                            sd_add_back_write(shared, make_view("client_chat_respond " + final_command));
                        }
                    }
                }
                else
                {
                    std::string escaped_string = escape_str(chat_win.command.command);

                    sized_string chat_command = sa_make_chat_command(make_view(chat_win.selected), make_view(escaped_string));

                    ///TODO
                    sd_add_back_write(shared, make_view(chat_command));

                    free_sized_string(chat_command);
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

                sized_string found_user = sd_get_user(shared);
                std::string data = handle_local_command(c_str_sized_to_cpp(found_user), cmd, term.auto_handle, should_shutdown, term);
                free_sized_string(found_user);

                term.add_text_from_server(shared, data, chat_win, false);

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
        else if(enter && to_edit->command.size() == 0)
        {
            sd_add_back_read(shared, make_view("command "));

            //to_edit->push_command_to_history(to_edit->command);
        }

        #ifdef TESTING
        ImGui::Begin("Debug_window");

        for(auto& i : api_calls)
        {
            ImGui::Text(i.c_str());
        }

        ImGui::End();
        #endif // TESTING

        chat_win.tick();

        auto sf_mpos = mouse.getPosition(window);
        vec2f vpos = {sf_mpos.x, sf_mpos.y};

        if(mouse.isButtonPressed(sf::Mouse::Left) && is_focused(focused))
        {
            inactivity_timer.restart();

            get_global_copy_handler()->on_hold_lclick(window,  vpos);
        }

        while(sd_has_front_read(shared))
        {
            inactivity_timer.restart();

            sized_string c_data = sd_get_front_read(shared);
            std::string fdata = c_str_sized_to_cpp(c_data);
            free_sized_string(c_data);

            #ifdef TESTING
            api_calls.push_back(fdata);
            #endif // TESTING

            term.add_text_from_server(shared, fdata, chat_win);
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

            text_editor.save_settings();
            //text_editor.save();

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
                sa_do_autocomplete_request(shared, make_view(str));

                break;
            }

            if(term.auto_handle.found_unprocessed_autocompletes.size() > 0)
                term.auto_handle.found_unprocessed_autocompletes.erase(term.auto_handle.found_unprocessed_autocompletes.begin());

            //term.auto_handle.found_unprocessed_autocompletes.clear();
        }

        if((term.focused || term.get_id_of_focused_realtime_window() != 1) && is_focused(focused) && key.isKeyPressed(sf::Keyboard::LControl) && ONCE_MACRO(sf::Keyboard::C))
        {
            sa_do_terminate_all_scripts(shared);
        }

        //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

        term.auto_handle.window_in_focus = is_focused(focused);
        chat_win.auto_handle.window_in_focus = is_focused(focused);

        //test_imgui_term.text_history = term.text_history;

        int was_closed_id = -1;

        //test_imgui_term.render(window);
        term.render_realtime_windows(font_context, shared, was_closed_id);
        chat_win.render(font_context, window, term.chat_threads, should_coordinate_focus);
        term.render(font_context, window, should_coordinate_focus);

        //window.display();
        //window.clear();

        should_coordinate_focus = false;

        if(was_closed_id != -1)
        {
            sa_do_terminate_script(shared, was_closed_id);
        }

        if(term.auto_handle.tab_pressed)
        {
            inactivity_timer.restart();
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

        /*sf::Text txt;
        txt.setFont(*font_select.get_base_sfml_font());
        txt.setString("Hi there hello there\nMy name is {3, 3}");
        txt.setCharacterSize(8);

        txt.setPosition(600, 650);
        window.draw(txt, sf::BlendAdd);*/

        //sf::Sprite spr(get_font_atlas());

        //window.draw(spr);

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
            inactivity_timer.restart();
            term.invalidate();
        }

        text_editor.set_current_user(c_str_consume(sd_get_user(shared)));
    }

    serialise sterm;
    sterm.handle_serialise(term, true);
    sterm.save(terminal_file);

    serialise swindow;
    swindow.handle_serialise(chat_win, true);
    swindow.save(chat_file);

    text_editor.save_only_modified();
    //text_editor.save(true);

    //sd_set_termination(shared);

    nc_shutdown(shared);

    CoUninitialize();

    return 0;
}
