#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "window_context.hpp"

#include <codecvt>
#include <locale>

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

#define DMAP(A) key_map[GLFW_KEY_##A] = make_lower((#A));
#define SMAP(A, S) key_map[GLFW_KEY_##A] = ((#S));
#define MMAP(A, S) mouse_map[GLFW_MOUSE_BUTTON_##A] = ((#S));

#define SPECIAL_MAP(A, S) on_input_map[GLFW_KEY_##A] = ((#S));

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

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

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

void handle_auth(c_steam_api csapi, connection& conn, std::string current_user)
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

        while(steam_api_should_wait_for_encrypted_token(csapi)){steam_api_pump_events(csapi);}

        if(!steam_api_has_encrypted_token(csapi))
        {
            printf("Failed to get encrypted token");
            throw std::runtime_error("Could not fetch steam token");
        }

        sized_string str = steam_api_get_encrypted_token(csapi);

        std::string etoken = c_str_consume(str);
        etoken = etoken;

        nlohmann::json data;
        data["type"] = "steam_auth";
        data["data"] = binary_to_hex(etoken);

        conn.write(data.dump());

        printf("Postwrite\n");
    }
    ///use key based auth
    else if(file_exists("key.key"))
    {
        printf("Pure key auth\n");

        nlohmann::json data;
        data["type"] = "key_auth";
        data["data"] = read_file_bin("key.key");

        conn.write(data.dump());
    }
    else
    {
        ///no auth available
        printf("No auth methods available, use steam or key.key file");
        throw std::runtime_error("No auth method available");
    }

    if(current_user.size() > 0)
    {
        nlohmann::json data;
        data["type"] = "generic_server_command";
        data["data"] = "user " + current_user;

        conn.write(data.dump());
    }
}

std::vector<int> glfw_mouse_pressed_data;
std::vector<int> glfw_mouse_released_data;

bool lastKeysDown[512] = {};

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        glfw_mouse_pressed_data.push_back(button);
    }

    if(action == GLFW_RELEASE)
    {
        glfw_mouse_released_data.push_back(button);
    }
}

bool just_pressed(int key)
{
    ImGuiIO& io = ImGui::GetIO();

    return io.KeysDown[key] && !lastKeysDown[key];
}

///relative to top left content area
vec2f cursor_pos;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto vpos = ImGui::GetMainViewport()->Pos;

    cursor_pos = (vec2f){xpos + vpos.x, ypos + vpos.y};
}

///test new repo
int main()
{
    //test();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    //stack_on_start();

    //token_tests();

    c_steam_api csapi = steam_api_alloc();

    connection conn;
    conn.connect(HOST_IP, HOST_PORT_SSL, connection_type::SSL);

    handle_auth(csapi, conn, "");

    glfwSetErrorCallback(glfw_error_callback);

    window_context window_ctx;

    ImFontAtlas atlas = {};

    ImGui::CreateContext(&atlas);

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    //ImGui::SetStyleLinearColor(window_ctx.is_srgb);

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

    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    glfwSetCursorPosCallback(window_ctx.window, cursor_position_callback);
    glfwSetMouseButtonCallback(window_ctx.window, mouse_button_callback);

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window_ctx.window, true);
    ImGui_ImplOpenGL3_Init(window_ctx.glsl_version);

    font_selector font_select;
    font_select.reset_default_fonts();

    terminal_imgui term;
    chat_window chat_win;

    bool should_coordinate_focus = true;

    std::map<int, std::string> key_map;
    std::map<int, std::string> on_input_map;
    std::map<int, std::string> mouse_map;

    DMAP(A);DMAP(B);DMAP(C);
    DMAP(D);DMAP(E);DMAP(F);
    DMAP(G);DMAP(H);DMAP(I);
    DMAP(J);DMAP(K);DMAP(L);
    DMAP(M);DMAP(N);DMAP(O);
    DMAP(P);DMAP(Q);DMAP(R);
    DMAP(S);DMAP(T);DMAP(U);
    DMAP(V);DMAP(W);DMAP(X);
    DMAP(Y);DMAP(Z);

    DMAP(0);DMAP(1);DMAP(2);
    DMAP(3);DMAP(4);DMAP(5);
    DMAP(6);DMAP(7);DMAP(8);
    DMAP(9);DMAP(0);

    SMAP(ENTER, return);
    SMAP(BACKSPACE, backspace);
    SMAP(DELETE, delete);

    SMAP(INSERT,insert);
    SMAP(TAB, tab);
    SMAP(DELETE, delete);

    SPECIAL_MAP(ENTER, return);
    SPECIAL_MAP(BACKSPACE, backspace);
    SPECIAL_MAP(DELETE, delete);

    SPECIAL_MAP(INSERT,insert);
    SPECIAL_MAP(TAB, tab);
    SPECIAL_MAP(DELETE, delete);

    SMAP(UP, up);
    SMAP(DOWN, down);
    SMAP(RIGHT, right);
    SMAP(LEFT, left);
    SMAP(HOME, home);
    SMAP(END, end);
    SMAP(PAGE_UP, pageup);
    SMAP(PAGE_DOWN, pagedown);

    SPECIAL_MAP(UP, up);
    SPECIAL_MAP(DOWN, down);
    SPECIAL_MAP(RIGHT, right);
    SPECIAL_MAP(LEFT, left);
    SPECIAL_MAP(HOME, home);
    SPECIAL_MAP(END, end);
    SPECIAL_MAP(PAGE_UP, pageup);
    SPECIAL_MAP(PAGE_DOWN, pagedown);

    SMAP(KP_1, 1);
    SMAP(KP_2, 2);
    SMAP(KP_3, 3);
    SMAP(KP_4, 4);
    SMAP(KP_5, 5);
    SMAP(KP_6, 6);
    SMAP(KP_7, 7);
    SMAP(KP_8, 8);
    SMAP(KP_9, 9);
    SMAP(KP_0, 0);

    SMAP(LEFT_SHIFT, lshift);
    SMAP(RIGHT_SHIFT, rshift);
    SMAP(LEFT_CONTROL, lctrl);
    SMAP(RIGHT_CONTROL, rctrl);
    SMAP(LEFT_ALT, lalt);
    SMAP(RIGHT_ALT, ralt);

    SPECIAL_MAP(LEFT_SHIFT, lshift);
    SPECIAL_MAP(RIGHT_SHIFT, rshift);
    SPECIAL_MAP(LEFT_CONTROL, lctrl);
    SPECIAL_MAP(RIGHT_CONTROL, rctrl);
    SPECIAL_MAP(LEFT_ALT, lalt);
    SPECIAL_MAP(RIGHT_ALT, ralt);

    SMAP(LEFT_BRACKET, [);
    SMAP(RIGHT_BRACKET, ]);

    key_map[GLFW_KEY_SEMICOLON] = ";";
    key_map[GLFW_KEY_COMMA] = ",";
    SMAP(PERIOD, .);
    SMAP(GRAVE_ACCENT, ~);
    SMAP(EQUAL, =);
    SMAP(MINUS, -);

    SMAP(SPACE, space);

    SMAP(ESCAPE, escape);
    SPECIAL_MAP(ESCAPE, escape);

    key_map[GLFW_KEY_APOSTROPHE] = "\'";
    SMAP(SLASH, /);
    SMAP(BACKSLASH, \\);

    MMAP(1, lmouse);
    MMAP(2, rmouse);
    MMAP(3, mmouse);

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
        if(glfwWindowShouldClose(window_ctx.window))
            running = false;

        /*if(font_select.update_rebuild(window, font_select.current_base_font_size))
        {
            term.invalidate();

            active_frames = active_frames_restart;
        }*/

        if(connection_clock.getElapsedTime().asSeconds() > 5 && !conn.client_connected_to_server)
        {
            conn.connect(HOST_IP, HOST_PORT_SSL, connection_type::SSL);
            connection_clock.restart();

            handle_auth(csapi, conn, current_user);

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

        ///needs reimpl
        /*if(active_frames <= 0)
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
        }*/

        std::vector<int> glfw_key_pressed_data;
        std::vector<int> glfw_key_released_data;

        glfw_mouse_pressed_data.clear();
        glfw_mouse_released_data.clear();

        memcpy(lastKeysDown, io.KeysDown, sizeof(lastKeysDown));

        glfwPollEvents();

        for(int i=0; i < sizeof(io.KeysDown) / sizeof(io.KeysDown[0]); i++)
        {
            if(io.KeysDown[i] && !lastKeysDown[i])
            {
                glfw_key_pressed_data.push_back(i);
            }

            if(!io.KeysDown[i] && lastKeysDown[i])
            {
                glfw_key_released_data.push_back(i);
            }
        }

        float scroll_y = io.MouseWheel;
        float scroll_x = io.MouseWheelH;

        std::vector<uint32_t> input_utf32;

        for(auto& i : io.InputQueueCharacters)
        {
            input_utf32.push_back(i);
        }

        for(uint32_t i : input_utf32)
        {
            if(i <= 126 && i >= 32)
            {
                std::u32string utf32;
                utf32.push_back(i);

                std::string utf8;

                std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;

                utf8 = cvt.to_bytes(utf32);

                to_edit->add_to_command(i);

                term.last_line_invalidate();

                std::string str = utf8;

                if(str == " ")
                    str = "space";

                realtime_str.push_back(str);
            }
        }

        for(int i : glfw_key_pressed_data)
        {
            if(on_input_map.find(i) != on_input_map.end())
                realtime_str.push_back(on_input_map[i]);

            if(key_map.find(i) != key_map.end())
                on_pressed.push_back(key_map[i]);

            term.last_line_invalidate();

            if(i == GLFW_KEY_BACKSPACE)
            {
                to_edit->process_backspace();
            }

            if(i == GLFW_KEY_DELETE)
            {
                to_edit->process_delete();
            }

            if(i == GLFW_KEY_UP)
            {
                to_edit->move_command_history_idx(-1);
            }

            if(i == GLFW_KEY_DOWN)
            {
                to_edit->move_command_history_idx(1);
            }

            if(i == GLFW_KEY_LEFT)
            {
                if(glfwGetKey(window_ctx.window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS)
                    to_edit->move_cursor(-1);
                else
                    to_edit->move_cursor(-5);
            }

            if(i == GLFW_KEY_RIGHT)
            {
                if(glfwGetKey(window_ctx.window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS)
                    to_edit->move_cursor(1);
                else
                    to_edit->move_cursor(5);
            }

            if(i == GLFW_KEY_HOME)
            {
                to_edit->move_cursor(-(int)to_edit->command.size());
            }

            if(i == GLFW_KEY_END)
            {
                to_edit->move_cursor(to_edit->command.size());
            }

            if(i == GLFW_KEY_ESCAPE)
            {
                to_edit->clear_command();
            }

            if(i == GLFW_KEY_V)
            {
                if(glfwGetKey(window_ctx.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && is_focused(focused))
                {
                    std::string add_text = get_clipboard_contents();

                    for(auto& i : add_text)
                    {
                        to_edit->add_to_command(i);
                    }
                }
            }

            if(i == GLFW_KEY_F1)
            {
                font_select.is_open = !font_select.is_open;
            }

            if(i == GLFW_KEY_ENTER)
            {
                if(to_edit != &realtime_shim && to_edit != &no_string)
                {
                    if(glfwGetKey(window_ctx.window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS && glfwGetKey(window_ctx.window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS)
                        enter = true;
                    else
                        to_edit->add_to_command('\n');
                }
                else
                    to_edit->add_to_command('\n');
            }

            ///TODO: DONT FORGET ABOUT FIXING TERMINAL INVALIDATION FOR THESE
            if(i == GLFW_KEY_PAGE_DOWN)
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

            if(i == GLFW_KEY_PAGE_UP)
            {
                if(hovered_string == &term.command)
                    mouse_delta += term.render_height - 2;
                if(hovered_string == &chat_win.command)
                    mouse_delta += chat_win.render_height - 2;

                term.last_line_invalidate();
            }
        }

        for(int i : glfw_key_released_data)
        {
            if(key_map.find(i) != key_map.end())
                on_released.push_back(key_map[i]);
        }

        for(int i : glfw_mouse_pressed_data)
        {
            if(mouse_map.find(i) != mouse_map.end())
                on_pressed.push_back(mouse_map[i]);

            if(mouse_map.find(i) != mouse_map.end())
                realtime_str.push_back(mouse_map[i]);

            if(i == GLFW_MOUSE_BUTTON_2)
            {
                std::string add_text = get_clipboard_contents();

                for(auto& i : add_text)
                {
                    to_edit->add_to_command(i);
                }

                term.last_line_invalidate();
            }

            if(i == GLFW_MOUSE_BUTTON_1)
            {
                term.last_line_invalidate();

                get_global_copy_handler()->on_lclick(cursor_pos);
            }
        }

        for(int i : glfw_mouse_released_data)
        {
            if(mouse_map.find(i) != mouse_map.end())
                on_released.push_back(mouse_map[i]);

            if(i == GLFW_MOUSE_BUTTON_1)
            {
                term.last_line_invalidate();

                get_global_copy_handler()->on_lclick_release(cursor_pos);
            }
        }

        mouse_delta += scroll_y;
        script_mousewheel_delta += scroll_y;

        #if 0
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
        #endif // 0

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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

            vec2f mpos = cursor_pos;

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

        ImGui::PushFont(font_select.get_base_font());

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

                term.add_text(data);

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

        if(glfwGetMouseButton(window_ctx.window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && is_focused(focused))
        {
            active_frames = active_frames_restart;

            get_global_copy_handler()->on_hold_lclick(cursor_pos);
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
            nlohmann::json data = nlohmann::json::parse(fdata);

            term.add_text_from_server(current_user, data, chat_win);
        }

        if(write_clock.getElapsedTime().asMilliseconds() > 5000)
        {
            atomic_write_all(terminal_file, serialise(term, serialise_mode::DISK).dump());
            atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK).dump());

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
        }

        if((term.focused || term.get_id_of_focused_realtime_window() != 1) && is_focused(focused) && glfwGetKey(window_ctx.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && just_pressed(GLFW_KEY_C))
        {
            nlohmann::json data;
            data["type"] = "client_terminate_scripts";
            data["id"] = -1;

            conn.write(data.dump());
        }

        //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

        term.auto_handle.window_in_focus = is_focused(focused);
        chat_win.auto_handle.window_in_focus = is_focused(focused);

        int was_closed_id = -1;

        int display_w, display_h;
        glfwGetFramebufferSize(window_ctx.window, &display_w, &display_h);

        //test_imgui_term.render(window);
        term.render_realtime_windows(conn, was_closed_id);
        chat_win.render(term.chat_threads, should_coordinate_focus);
        term.render({display_w, display_h}, should_coordinate_focus);

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

        term.auto_handle.tab_pressed = just_pressed(GLFW_KEY_TAB) && is_focused(focused);

        if(term.auto_handle.tab_pressed)
        {
            term.last_line_invalidate();
        }

        ///this is a hack to fix the fact that sometimes
        ///click input doesn't make clean click/release pairs
        if(glfwGetMouseButton(window_ctx.window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        {
            get_global_copy_handler()->finished = false;
            get_global_copy_handler()->held = false;
        }

        ImGui::PopFont();

        ImGui::Render();

        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window_ctx.window);

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

    atomic_write_all(terminal_file, serialise(term, serialise_mode::DISK).dump());
    atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK).dump());

    CoUninitialize();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_ctx.window);
    glfwTerminate();

    return 0;
}
