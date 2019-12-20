#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
//#include "window_context.hpp"
#include <toolkit/render_window.hpp>
#include <toolkit/base_serialisables.hpp>
#include <functional>

#include <codecvt>
#include <locale>

#include <vec/vec.hpp>

#include "util.hpp"

#include "colour_interop.hpp"
#include "string_helpers.hpp"

#include "auto_handlers.hpp"
#include "copy_handler.hpp"
#include "editable_string.hpp"
#include "tokeniser.hpp"
#include "serialisables.hpp"
#include <networking/networking.hpp>
#include <cstdlib>

///ruh roh
///need to structure this project properly
#include "local_commands.hpp"
#include <libncclient/nc_util.hpp>
#include "steam_api.hpp"

#include <imgui/imgui.h>

#include "imgui_ui_components.hpp"
#include <nlohmann/json.hpp>
#include <stdio.h>
#include "font_cfg.hpp"
#include <iomanip>
#include "imguix.hpp"
#include <misc/cpp/imgui_stdlib.h>

#ifdef __EMSCRIPTEN__
  #include <emscripten/emscripten.h>
  #include <emscripten/html5.h>
#endif // __EMSCRIPTEN__

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
#define MMAP(A, S) mouse_map[A] = ((#S));

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

void pretty_atomic_write_all(const std::string& file, const nlohmann::json& js)
{
    atomic_write_all(file, js.dump(1));
}

std::string default_up_handling(const std::string& user, const std::string& server_msg, const std::string& scripts_dir)
{
    std::string up = "#up ";
    std::string up_es6 = "#up_es6 ";
    std::string dry = "#dry ";
    std::string up_es5 = "#up_es5 ";

    std::vector<std::string> strings = no_ss_split(server_msg, " ");

    if((starts_with(server_msg, up) || starts_with(server_msg, dry) || starts_with(server_msg, up_es5)) && strings.size() == 2)
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

        if(starts_with(server_msg, up_es5))
        {
            comm = up;
        }

        return comm + name + " " + data;
    }

    return server_msg;
}

void handle_auth(steamapi& s_api, connection& conn, std::string current_user)
{
    //if(!conn.client_connected_to_server)
    //    return;

    if(s_api.enabled)
    {
        ///embed hex_key.key
        if(file_exists("hey_key.key"))
        {
            printf("Embedding key auth in steam auth\n");

            s_api.request_auth_token(read_file_bin("hex_key.key"));
        }
        else
        {
            printf("Steam auth, no key auth");

            s_api.request_auth_token("");
        }

        while(s_api.should_wait_for_encrypted_token())
        {
            s_api.pump_callbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if(!s_api.auth_success())
        {
            printf("Failed to get encrypted token");
            throw std::runtime_error("Could not fetch steam token");
        }

        auto res = s_api.get_encrypted_token();

        std::string etoken = std::string(res.begin(), res.end());

        nlohmann::json data;
        data["type"] = "steam_auth";
        data["data"] = binary_to_hex(etoken);

        conn.write(data.dump());

        printf("Postwrite\n");
    }
    ///use key based auth
    else if(file_exists("hex_key.key"))
    {
        printf("Pure key auth\n");

        nlohmann::json data;
        data["type"] = "key_auth";
        data["data"] = read_file_bin("hex_key.key");

        conn.write(data.dump());
    }
    else
    {
        #ifndef __EMSCRIPTEN__
        ///no auth available
        printf("No auth methods available, use steam or hex_key.key file");
        throw std::runtime_error("No auth method available");
        #else
        nlohmann::json data;
        data["type"] = "key_auth";
        data["data"] = "5FE7F90855A578F4184356954DEB0DA389C4D69672884F01FD27E72CAD89EBB54D9A92EE5E36B4F96FB6A53403012406A41270A80BE7035D167DB2550C3CCE89EA010E1FC403788AD43B068460C5B762944D0A2F10377D6B021E121B4D75C4AAD0990D5445431F0522090FB2862E290C986F53C60B3EE42C7F8E6038475F1BD1";

        conn.write(data.dump());
        #endif
    }

    if(current_user.size() > 0)
    {
        nlohmann::json data;
        data["type"] = "generic_server_command";
        data["data"] = "user " + current_user;

        conn.write(data.dump());
    }
}

std::function<void()> hptr;

void main_loop_helper(void* ptr)
{
    hptr();
}

///test new repo
int main(int argc, char* argv[])
{
    //token_tests();

    bool no_viewports = false;

    if(argc > 1)
    {
        for(int i=1; i < argc; i++)
        {
            std::string sarg = argv[i];

            if(sarg == "-noviewports" || sarg == "-noviewport")
            {
                no_viewports = true;

                printf("Viewports are disabled\n");
            }
        }
    }

    steamapi s_api;

    printf("Pre connect\n");

    connection conn;
    #ifndef __EMSCRIPTEN__
    conn.connect(HOST_IP, HOST_PORT_SSL, connection_type::SSL);
    #else
    conn.connect(HOST_IP, HOST_PORT, connection_type::PLAIN);
    #endif

    printf("Post Connect\n");

    handle_auth(s_api, conn, "");

    render_settings sett;

    bool has_file = false;

    std::string window_file = "./window_v2.txt";

    if(file_exists(window_file))
    {
        try
        {
            nlohmann::json window_nlohmann = load_from_file_json(window_file);

            deserialise(window_nlohmann, sett, serialise_mode::DISK);

            has_file = true;
        }
        catch(...)
        {

        }
    }

    sett.viewports = !no_viewports;

    if(!has_file)
    {
        sett.width = 1400;
        sett.height = 900;

        sett.is_srgb = true;
    }

    render_window window(sett, "net_code");

    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigViewportsNoTaskBarIcon = true;

    ImGui::PushSrgbStyleColor(ImGuiCol_WindowBg, ImGuiX::GetBgCol());

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 0;
    style.WindowRounding = 0;
    style.ChildRounding = 0;
    style.ChildBorderSize = 0;
    style.FrameBorderSize = 0;
    //style.PopupBorderSize = 0;
    style.WindowBorderSize = 1;

    printf("Done linear colours and styles\n");

    /*style.FramePadding = ImVec2(0,0);
    style.DisplaySafeAreaPadding = ImVec2(0,0);
    style.DisplayWindowPadding = ImVec2(0,0);
    style.ItemInnerSpacing = ImVec2(0,0);*/

    font_selector font_select;
    font_select.reset_default_fonts();

    terminal_imgui term;
    chat_window chat_win;

    printf("Terminal + Chat window\n");

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

    MMAP(0, lmouse);
    MMAP(1, rmouse);
    MMAP(2, mmouse);

    printf("Specials\n");

    std::string terminal_file = "./terminal_v6.txt";
    std::string chat_file = "./chat_v6.txt";
    //std::string settings_file = "./text_sett_v1.txt";
    std::string font_file = "./font_sett_v1.txt";
    std::string notepad_file = "./notepad.txt";

    try
    {
        if(file_exists(terminal_file))
        {
            nlohmann::json dat = load_from_file_json(terminal_file);
            deserialise(dat, term, serialise_mode::DISK);
        }
    }
    catch(...){}

    try
    {
        if(file_exists(chat_file))
        {
            nlohmann::json dat = load_from_file_json(chat_file);
            deserialise(dat, chat_win, serialise_mode::DISK);
        }
    }
    catch(...){}

    try
    {
        if(file_exists(font_file))
        {
            nlohmann::json dat = load_from_file_json(font_file);
            deserialise(dat, font_select, serialise_mode::DISK);
        }
    }
    catch(...){}

    std::string notepad;

    if(file_exists(notepad_file))
    {
        notepad = read_file(notepad_file);
    }

    printf("Loaded files\n");

    steady_timer render_clock;
    steady_timer request_clock;
    steady_timer write_clock;
    steady_timer imgui_delta;

    steady_timer mouse_clock;
    float mouse_send_time_ms = 33;

    #ifdef TESTING
    std::vector<std::string> api_calls;
    #endif // TESTING

    //double diff_s = 0.f;

    editable_string realtime_shim;
    std::vector<std::string> realtime_str;
    std::vector<std::string> on_pressed;
    std::vector<std::string> on_released;

    double script_mousewheel_delta = 0.;

    invalidate_everything(term, chat_win);

    std::string current_user = "";

    steady_timer connection_clock;

    bool lastKeysDown[512] = {};
    bool lastMouseDown[5] = {};
    ImVec2 last_mouse_pos = ImVec2(0,0);
    ImVec2 last_display_size = ImVec2(0,0);

    auto just_pressed = [&](int key)
    {
        ImGuiIO& io = ImGui::GetIO();

        return io.KeysDown[key] && !lastKeysDown[key];
    };

    bool has_settings_window = false;

    printf("Pre main loop\n");

    ImGuiIO& io = ImGui::GetIO();

    int iconn = conn.client_connected_to_server;
    printf("Am connected? %i\n", iconn);

    //while(running)
    #ifndef __EMSCRIPTEN__
    while(!window.should_close())
    #else
    hptr = [&]()
    #endif
    {
        if(connection_clock.get_elapsed_time_s() > 5 && !conn.client_connected_to_server && !conn.connection_pending())
        {
            #ifndef __EMSCRIPTEN__
            conn.connect(HOST_IP, HOST_PORT_SSL, connection_type::SSL);
            #else
            conn.connect(HOST_IP, HOST_PORT, connection_type::PLAIN);
            #endif

            connection_clock.restart();

            handle_auth(s_api, conn, current_user);

            term.add_text("Connecting...");
        }

        realtime_shim.clear_command();
        realtime_str.clear();
        on_pressed.clear();
        on_released.clear();

        editable_string no_string;
        editable_string* to_edit = &no_string;

        if(term.focused)
            to_edit = &term.command;

        bool has_chat_window = chat_win.focused;

        if(chat_win.focused && has_chat_window)
            to_edit = chat_win.get_focused_editable().value_or(&no_string);

        if(term.get_id_of_focused_realtime_window() != -1)
            to_edit = &realtime_shim;

        bool enter = false;

        float mouse_delta = 0.f;

        bool skip_first_event = false;

        s_api.pump_callbacks();

        std::vector<int> glfw_key_pressed_data;
        std::vector<int> glfw_key_released_data;

        std::vector<int> mouse_pressed_data;
        std::vector<int> mouse_released_data;

        bool any_events = false;

        any_events = any_events || (memcmp(lastKeysDown, io.KeysDown, sizeof(lastKeysDown)) != 0) || (memcmp(lastMouseDown, io.MouseDown, sizeof(lastMouseDown)) != 0);

        for(int i=0; i < 512; i++)
        {
            if(io.KeysDown[i])
                any_events = true;
        }

        for(int i=0; i < 5; i++)
        {
            if(io.MouseDown[i])
                any_events = true;
        }

        memcpy(lastKeysDown, io.KeysDown, sizeof(lastKeysDown));
        memcpy(lastMouseDown, io.MouseDown, sizeof(io.MouseDown));

        static_assert(sizeof(lastKeysDown) == sizeof(io.KeysDown));
        static_assert(sizeof(lastMouseDown) == sizeof(io.MouseDown));

        #ifndef __EMSCRIPTEN__
        window.poll(1/33.);
        #else
        window.poll_events_only();
        #endif

        any_events = any_events || (memcmp(lastKeysDown, io.KeysDown, sizeof(lastKeysDown)) != 0) || (memcmp(lastMouseDown, io.MouseDown, sizeof(lastMouseDown)) != 0);

        any_events = any_events || (io.MousePos.x != last_mouse_pos.x || io.MousePos.y != last_mouse_pos.y);
        any_events = any_events || (io.MouseDelta.x != 0 || io.MouseDelta.y != 0);
        any_events = any_events || (io.MouseWheel != 0 || io.MouseWheelH != 0);

        any_events = any_events || (io.DisplaySize.x != last_display_size.x || io.DisplaySize.y != last_display_size.y);

        any_events = any_events || conn.has_read();

        //printf("Any events? %i %f %f\n", any_events, io.MousePos.x, io.MousePos.y);

        last_display_size = io.DisplaySize;
        last_mouse_pos = io.MousePos;

        #ifdef __EMSCRIPTEN__
        if(any_events)
        #endif // __EMSCRIPTEN__
        {
            #ifdef __EMSCRIPTEN__
            window.poll_issue_new_frame_only();
            #endif // __EMSCRIPTEN__

            /*if(font_select.update_rebuild(window, font_select.current_base_font_size))
            {
                term.invalidate();
            }*/

            vec2f cursor_pos = {io.MousePos.x, io.MousePos.y};

            for(int i=0; i < sizeof(io.KeysDown) / sizeof(io.KeysDown[0]); i++)
            {
                if((io.KeysDown[i] && !lastKeysDown[i]) || ImGui::IsKeyPressed(i))
                {
                    glfw_key_pressed_data.push_back(i);
                }

                if(!io.KeysDown[i] && lastKeysDown[i])
                {
                    glfw_key_released_data.push_back(i);
                }
            }

            for(int i=0; i < sizeof(io.MouseDown) / sizeof(io.MouseDown[0]); i++)
            {
                if(io.MouseDown[i] && !lastMouseDown[i])
                {
                    mouse_pressed_data.push_back(i);
                }

                if(!io.MouseDown[i] && lastMouseDown[i])
                {
                    mouse_released_data.push_back(i);
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

                    last_line_invalidate_everything(term, chat_win);

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

                last_line_invalidate_everything(term, chat_win);

                if(i == GLFW_KEY_BACKSPACE)
                {
                    if(ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
                    {
                        for(int i=0; i < 5; i++)
                            to_edit->process_backspace();
                    }
                    else
                    {
                        to_edit->process_backspace();
                    }
                }

                if(i == GLFW_KEY_DELETE)
                {
                    if(ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
                    {
                        for(int i=0; i < 5; i++)
                            to_edit->process_delete();
                    }
                    else
                    {
                        to_edit->process_delete();
                    }
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
                    if(!ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
                        to_edit->move_cursor(-1);
                    else
                        to_edit->move_cursor(-5);
                }

                if(i == GLFW_KEY_RIGHT)
                {
                    if(!ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
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
                    if(ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
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

                if(i == GLFW_KEY_ENTER || i == GLFW_KEY_KP_ENTER)
                {
                    if(to_edit != &realtime_shim && to_edit != &no_string)
                    {
                        if(!ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && !ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT))
                            enter = true;
                        else
                            to_edit->add_to_command('\n');
                    }
                    else
                        to_edit->add_to_command('\n');
                }
            }

            for(int i : glfw_key_released_data)
            {
                if(key_map.find(i) != key_map.end())
                    on_released.push_back(key_map[i]);
            }

            for(int i : mouse_pressed_data)
            {
                if(mouse_map.find(i) != mouse_map.end())
                    on_pressed.push_back(mouse_map[i]);

                if(mouse_map.find(i) != mouse_map.end())
                    realtime_str.push_back(mouse_map[i]);

                if(i == 1)
                {
                    std::string add_text = get_clipboard_contents();

                    for(auto& i : add_text)
                    {
                        to_edit->add_to_command(i);
                    }

                    last_line_invalidate_everything(term, chat_win);
                }

                if(i == 0)
                {
                    last_line_invalidate_everything(term, chat_win);

                    get_global_copy_handler()->on_lclick(cursor_pos);
                }
            }

            for(int i : mouse_released_data)
            {
                if(mouse_map.find(i) != mouse_map.end())
                    on_released.push_back(mouse_map[i]);

                if(i == 0)
                {
                    last_line_invalidate_everything(term, chat_win);

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

                if(event.type == sf::Event::GainedFocus)
                {
                    focused = true;

                    term.invalidate();
                }

                if(event.type == sf::Event::LostFocus)
                    focused = false;

                if(event.type == sf::Event::Resized)
                {
                    window.setSize({event.size.width, event.size.height});
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));

                    term.invalidate();

                    window_ctx.width = event.size.width;
                    window_ctx.height = event.size.height;

                    window_ctx.save();
                }
            }
            #endif // 0

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

            if(term.get_id_of_focused_realtime_window() != -1 && mouse_clock.get_elapsed_time_s() * 1000 >= mouse_send_time_ms)
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

            if(term.get_id_of_focused_realtime_window() == -1)
                script_mousewheel_delta = 0;

            term.scroll_hack.scrolled_this_frame = mouse_delta;
            chat_win.scroll_hack.scrolled_this_frame = mouse_delta;

            for(auto& i : term.realtime_script_windows)
            {
                i.second.scroll_hack.scrolled_this_frame = mouse_delta;
            }

            ImGui::PushFont(font_select.get_base_font());

            font_select.render(window);

            /*if(window_ctx.srgb_dirty)
            {
                ImGui::SetStyleLinearColor(window_ctx.is_srgb);
            }*/

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
            }

            if(enter && to_edit->command.size() > 0)
            {
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
                    term.cache.invalidate();

                    if(!is_local_command(term.command.command))
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
                    std::string command = "";

                    std::optional<chat_thread*> thrd = chat_win.get_focused_chat_thread();

                    if(thrd.has_value())
                    {
                        command = thrd.value()->command.command;
                        thrd.value()->cache.invalidate();
                    }

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
                    else if(has_chat_window)
                    {
                        std::optional<chat_thread*> thrd = chat_win.get_focused_chat_thread();

                        if(thrd.has_value())
                        {
                            std::string escaped_string = escape_str(thrd.value()->command.command);

                            nlohmann::json data;
                            data["type"] = "client_chat";
                            data["data"] = "#hs.msg.send({channel:\"" + thrd.value()->name + "\", msg:\"" + escaped_string + "\"})";

                            conn.write(data.dump());
                        }
                    }

                    if(bump)
                    {
                        chat_win.add_text_to_focused(command);

                        std::optional<chat_thread*> thrd = chat_win.get_focused_chat_thread();

                        if(thrd)
                        {
                            thrd.value()->cache.invalidate();
                        }
                    }
                }

                std::string cmd = term.command.command;

                if(term.focused)
                {
                    term.bump_command_to_history();
                }
                else if(auto opt = chat_win.get_focused_chat_thread(); opt.has_value())
                {
                    opt.value()->command.clear_command();
                }

                if(term.focused && is_local_command(cmd))
                {
                    bool should_shutdown = false;

                    std::string data = handle_local_command(current_user, cmd, term.auto_handle, should_shutdown, term, chat_win);

                    term.add_text(data);

                    if(should_shutdown)
                    {
                        window.close();
                    }
                }

                pretty_atomic_write_all(terminal_file, serialise(term, serialise_mode::DISK));
                pretty_atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK));
            }
            else if(enter && to_edit->command.size() == 0)
            {
                term.add_text(" ");
            }

            if(ImGui::IsMouseDown(0))
            {
                get_global_copy_handler()->on_hold_lclick(cursor_pos);
            }

            while(conn.has_read())
            {
                write_data dat = conn.read_from();

                std::string fdata = dat.data;

                conn.pop_read(dat.id);

                #ifdef TESTING
                api_calls.push_back(fdata);
                #endif // TESTING

                ///this is temporary before the other end of the api gets changed
                nlohmann::json data = nlohmann::json::parse(fdata);

                term.add_text_from_server(current_user, data, chat_win, font_select);
            }

            if(write_clock.get_elapsed_time_s() > 5)
            {
                pretty_atomic_write_all(terminal_file, serialise(term, serialise_mode::DISK));
                pretty_atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK));

                auto save_sett = window.get_render_settings();

                pretty_atomic_write_all(window_file, serialise(save_sett, serialise_mode::DISK));

                write_clock.restart();
            }

            ///hmm
            ///this is inadequate
            ///we need to be able to request multiple scripts at once
            ///and receive multiple as well
            if(term.auto_handle.found_unprocessed_autocompletes.size() > 0 && request_clock.get_elapsed_time_s() > 0.3)
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

            if((term.focused || term.get_id_of_focused_realtime_window() != 1) && ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && just_pressed(GLFW_KEY_C))
            {
                nlohmann::json data;
                data["type"] = "client_terminate_scripts";
                data["id"] = -1;

                conn.write(data.dump());
            }

            if(has_settings_window)
            {
                ImGui::Begin("Settings", &has_settings_window);

                ImGui::Checkbox("Show chat in terminal", &chat_win.show_chat_in_main_window);

                ImGui::End();
            }

            {
                ImGui::Begin("notepad");

                bool changed = ImGui::InputTextMultiline("Doot", &notepad, ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_None);

                if(changed)
                {
                    atomic_write_all(notepad_file, notepad);
                }

                ImGui::End();
            }

            if(ImGui::IsKeyPressed(GLFW_KEY_F3))
                has_settings_window = !has_settings_window;

            //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

            term.auto_handle.window_in_focus = true;
            chat_win.auto_handle.window_in_focus = true;

            int was_closed_id = -1;

            vec2i window_dim = window.get_window_size();

            //test_imgui_term.render(window);
            term.render_realtime_windows(conn, was_closed_id, font_select);
            chat_win.render(should_coordinate_focus);
            term.render({window_dim.x(), window_dim.y()}, should_coordinate_focus);

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
                last_line_invalidate_everything(term, chat_win);
            }

            term.auto_handle.tab_pressed = ImGui::IsKeyPressed(GLFW_KEY_TAB);

            if(term.auto_handle.tab_pressed)
            {
                last_line_invalidate_everything(term, chat_win);
            }

            ///this is a hack to fix the fact that sometimes
            ///click input doesn't make clean click/release pairs
            if(!ImGui::IsMouseDown(0))
            {
                get_global_copy_handler()->finished = false;
                get_global_copy_handler()->held = false;
            }


            int lcwidth = char_inf::cwidth;
            int lcheight = char_inf::cheight;

            char_inf::cwidth = ImGui::CalcTextSize("A").x + char_inf::extra_glyph_spacing;
            char_inf::cheight = ImGui::CalcTextSize("A").y;

            if(char_inf::cwidth != lcwidth || char_inf::cheight != lcheight)
            {
                invalidate_everything(term, chat_win);
            }

            ImGui::PopFont();
            window.display();
        }
        #ifdef __EMSCRIPTEN__
        else
        {
            window.display_last_frame();
        }
        #endif // __EMSCRIPTEN__
    };

    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg((em_arg_callback_func)main_loop_helper, nullptr, 0, 1);
    #endif

    atomic_write_all(notepad_file, notepad);
    pretty_atomic_write_all(terminal_file, serialise(term, serialise_mode::DISK));
    pretty_atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK));

    auto save_sett = window.get_render_settings();

    pretty_atomic_write_all(window_file, serialise(save_sett, serialise_mode::DISK));

    return 0;
}
