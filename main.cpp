#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
//#include "window_context.hpp"
#include <toolkit/render_window.hpp>
#include <toolkit/render_window_sdl2.hpp>
#include <toolkit/render_window_glfw.hpp>
#include <toolkit/base_serialisables.hpp>
#include <toolkit/clipboard.hpp>
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
#include "auth_manager.hpp"

///ruh roh
///need to structure this project properly
#include "local_commands.hpp"
#include <libncclient/nc_util.hpp>
#include <toolkit/fs_helpers.hpp>
#include "steam_api.hpp"

#include <imgui/imgui.h>

#include "imgui_ui_components.hpp"
#include <nlohmann/json.hpp>
#include <stdio.h>
#include "font_cfg.hpp"
#include <iomanip>
#include <misc/cpp/imgui_stdlib.h>
#include <toolkit/fs_helpers.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
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

//#define HOST_IP "192.168.0.55"
#ifdef EXTERN_IP
//#define HOST_IP "77.96.132.101"
#define HOST_IP "netcodegame.com"
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_IP "127.0.0.1"
#endif // LOCAL_IP

#ifdef EXTERN_IP
#define HOST_PORT 6760
#define HOST_PORT_SSL 2096
//#define HOST_PORT_SSL 6780
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_PORT 6761
#define HOST_PORT_SSL 6781
#endif // LOCAL_IP

void pretty_atomic_write_all(const std::string& file, const nlohmann::json& js)
{
    file::write(file, js.dump(1), file::mode::BINARY);
}

std::string default_up_handling(const std::string& user, const std::string& server_msg, const std::string& scripts_dir)
{
    std::string up = "#up ";
    std::string dry = "#dry ";

    std::vector<std::string> strings = no_ss_split(server_msg, " ");

    if((starts_with(server_msg, up) || starts_with(server_msg, dry)) && strings.size() == 2)
    {
        std::string name = strings[1];

        std::string diskname = scripts_dir + name + ".js";

        std::string comm = up;

        if(starts_with(server_msg, dry))
            comm = dry;

        std::string data = "";

        if(file::exists(diskname))
            data = file::read(diskname, file::mode::TEXT);

        return comm + name + " " + data;
    }

    return server_msg;
}

#ifdef __EMSCRIPTEN__
EM_JS(int, get_window_location_length, (),
{
    return window.location.hostname.length + window.location.pathname.length;
});

EM_JS(void, get_window_location, (char* out, int len),
{
    stringToUTF8(window.location.hostname + window.location.pathname, out, len);
});

EM_JS(bool, is_window_visible, (),
{
    return !document.hidden;
});
#else
bool is_window_visible()
{
    return true;
}
#endif // __EMSCRIPTEN__

std::function<void()> hptr;

void main_loop_helper(void* ptr)
{
    hptr();
}

///test new repo
int main(int argc, char* argv[])
{
    //token_tests();

    file::init();

    bool no_viewports = false;

    std::string connection_ip = HOST_IP;
    uint64_t connection_port = HOST_PORT_SSL;

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

            if(i != argc - 1)
            {
                if(sarg == "-serverip")
                {
                    connection_ip = argv[i + 1];
                    i++;
                    continue;
                }

                if(sarg == "-serverport")
                {
                    connection_port = std::stoi(argv[i + 1]);
                    i++;
                    continue;
                }
            }
        }
    }

    steamapi s_api;

    printf("Pre connect\n");

    #ifdef __EMSCRIPTEN__
    {
        int window_location_len = get_window_location_length();
        std::string location_buf;
        location_buf.resize(window_location_len + 1);
        get_window_location(&location_buf[0], window_location_len + 1);

        int cstrlen = strlen(location_buf.c_str());
        location_buf = std::string(location_buf.begin(), location_buf.begin() + cstrlen);

        connection_ip = location_buf;

        printf("Redirected connection to %s\n", connection_ip.c_str());
    }
    #endif // __EMSCRIPTEN__

    connection conn;
    #ifndef __EMSCRIPTEN__
    conn.connect(connection_ip, connection_port, connection_type::SSL, "netcodegame.com");
    #else
    conn.set_client_sleep_interval(8);
    conn.connect(connection_ip, connection_port, connection_type::EMSCRIPTEN_AUTOMATIC);
    #endif

    connection_send_data to_write(conn.get_settings());
    connection_received_data to_read;

    printf("Post Connect\n");

    render_settings sett;

    auth_manager auth_manage;

    auth_manage.check(s_api, to_write, "");
    conn.send_bulk(to_write);

    bool has_file = false;

    std::string window_file = "window_v2.txt";

    if(file::exists(window_file))
    {
        try
        {
            nlohmann::json window_nlohmann = nlohmann::json::parse(file::read(window_file, file::mode::BINARY));

            deserialise(window_nlohmann, sett, serialise_mode::DISK);

            has_file = true;
        }
        catch(...)
        {
            printf("Invalid window settings\n");
        }
    }

    sett.viewports = !no_viewports;
    sett.no_decoration = true;

    if(!has_file)
    {
        sett.width = 1400;
        sett.height = 900;

        sett.is_srgb = true;
        sett.vsync = true;
    }

    generic_backend* backend = new sdl2_backend(sett, "net_code_");

    render_window window(sett, backend);
    ImGui::GetIO().MouseDragThreshold = 0;
    //ImGui::GetIO().ConfigWindowsResizeFromEdges = false;

    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigViewportsNoTaskBarIcon = true;

    ImVec4 background_col(30/255.f, 30/255.f, 30/255.f, 1.f);

    ImGui::PushSrgbStyleColor(ImGuiCol_WindowBg, background_col);

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

    terminal_manager terminals;

    chat_window chat_win;
    realtime_script_manager realtime_scripts;

    printf("Terminal + Chat window\n");

    bool should_coordinate_focus = true;

    ///platform independent
    std::map<int, std::string> mouse_map;
    mouse_map[0] = "lmouse";
    mouse_map[1] = "rmouse";
    mouse_map[2] = "mmouse";
    mouse_map[3] = "x1mouse";
    mouse_map[4] = "x2mouse";

    printf("Specials\n");

    std::string terminal_file = "terminal_v7.txt";
    std::string chat_file = "chat_v6.txt";
    //std::string settings_file = "text_sett_v1.txt";
    std::string font_file = "font_sett_v1.txt";
    std::string notepad_file = "notepad.txt";

    try
    {
        if(file::exists(terminal_file))
        {
            nlohmann::json dat = nlohmann::json::parse(file::read(terminal_file, file::mode::BINARY));
            deserialise(dat, terminals, serialise_mode::DISK);
        }
    }
    catch(...){printf("Invalid terminal file\n");}

    try
    {
        if(file::exists(chat_file))
        {
            nlohmann::json dat = nlohmann::json::parse(file::read(chat_file, file::mode::BINARY));
            deserialise(dat, chat_win, serialise_mode::DISK);
        }
    }
    catch(...){printf("Invalid chat file\n");}

    try
    {
        if(file::exists(font_file))
        {
            nlohmann::json dat = nlohmann::json::parse(file::read(font_file, file::mode::BINARY));
            deserialise(dat, font_select, serialise_mode::DISK);
            font_select.find_saved_font();
        }
    }
    catch(...){printf("Invalid font file\n");}

    std::string notepad;

    if(file::exists(notepad_file))
    {
        notepad = file::read(notepad_file, file::mode::BINARY);
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

    invalidate_everything(terminals, chat_win);

    std::string current_user = "";

    steady_timer connection_clock;

    bool lastKeysDown[512] = {};
    bool lastMouseDown[5] = {};
    bool curKeysDown[512] = {};
    bool curMouseDown[5] = {};
    ImVec2 last_mouse_pos = ImVec2(0,0);
    ImVec2 last_display_size = ImVec2(0,0);

    auto just_pressed = [&](int key)
    {
        return curKeysDown[key] && !lastKeysDown[key];
    };

    bool has_settings_window = false;

    printf("Pre main loop\n");

    ImGuiIO& io = ImGui::GetIO();

    int iconn = conn.client_connected_to_server;
    printf("Am connected? %i\n", iconn);

    int max_unprocessed_frames = 200;
    int unprocessed_frames = max_unprocessed_frames;
    bool last_item_active = false;

    steady_timer sleep_limiter;

    bool printed_connecting = false;

    //while(running)
    #ifndef __EMSCRIPTEN__
    while(!window.should_close() && terminals.main_terminal.open)
    #else
    hptr = [&]()
    #endif
    {
        conn.receive_bulk(to_read);

        #ifdef __EMSCRIPTEN__
        int sleep_time = is_window_visible() ? 8 : 64;
        conn.set_client_sleep_interval(sleep_time);
        #endif // __EMSCRIPTEN__

        file::manual_fs_sync manual_sync;

        bool visual_events = false;

        if(connection_clock.get_elapsed_time_s() > 5 && !conn.client_connected_to_server && !conn.connection_pending())
        {
            #ifndef __EMSCRIPTEN__
            conn.connect(connection_ip, connection_port, connection_type::SSL, "netcodegame.com");
            #else
            conn.connect(connection_ip, connection_port, connection_type::EMSCRIPTEN_AUTOMATIC);
            #endif

            connection_clock.restart();

            auth_manage.check(s_api, to_write, current_user);

            if(!printed_connecting)
                terminals.main_terminal.add_text("Connecting...", terminals.auto_handle);
            else
                terminals.main_terminal.extend_text(".", terminals.auto_handle);

            printed_connecting = true;
        }

        if(conn.client_connected_to_server)
        {
            printed_connecting = false;
        }

        realtime_shim.clear_command();
        realtime_str.clear();
        on_pressed.clear();
        on_released.clear();

        editable_string no_string;
        editable_string* to_edit = &no_string;

        if(terminals.get_focused_terminal()->focused)
            to_edit = &terminals.get_focused_terminal()->command;

        bool has_chat_window = chat_win.focused;

        if(chat_win.focused && has_chat_window)
            to_edit = chat_win.get_focused_editable().value_or(&no_string);

        if(realtime_scripts.get_id_of_focused_realtime_window() != -1)
            to_edit = &realtime_shim;

        bool enter = false;

        float mouse_delta = 0.f;

        bool last_is_open = s_api.is_overlay_open();

        s_api.pump_callbacks();

        std::vector<int> glfw_key_pressed_data;
        std::vector<int> glfw_key_released_data;

        std::vector<int> mouse_pressed_data;
        std::vector<int> mouse_released_data;

        static_assert(sizeof(lastKeysDown) == sizeof(curKeysDown));
        static_assert(sizeof(lastMouseDown) == sizeof(curMouseDown));

        memcpy(lastKeysDown, curKeysDown, sizeof(lastKeysDown));
        memcpy(lastMouseDown, curMouseDown, sizeof(curMouseDown));
        /*#ifndef __EMSCRIPTEN__
        window.poll(1/33.);
        #else
        window.poll_events_only();
        #endif*/

        ///can't just ramp up sleep... because glfw has no understanding of server events
        double max_sleep = 1/128.;

        if(s_api.is_overlay_open())
        {
            max_sleep = 1/128.;
        }

        //#define NO_SLEEP
        #ifndef NO_SLEEP
        window.poll_events_only(max_sleep);
        #else
        window.poll_events_only(0);
        #endif // NO_SLEEP

        memcpy(curKeysDown, io.KeysDown, sizeof(curKeysDown));
        memcpy(curMouseDown, io.MouseDown, sizeof(curMouseDown));

        for(int i=0; i < 512; i++)
        {
            if(curKeysDown[i] || (lastKeysDown[i] != curKeysDown[i]) || ImGui::IsKeyPressed(i) || ImGui::IsKeyReleased(i))
                visual_events = true;
        }

        for(int i=0; i < 5; i++)
        {
            if(curMouseDown[i] || (lastMouseDown[i] != curMouseDown[i]) || ImGui::IsMouseClicked(i) || ImGui::IsMouseReleased(i))
                visual_events = true;
        }

        //bool can_suppress_inputs = false;

        bool has_mouse_delta = false;

        has_mouse_delta = has_mouse_delta || (io.MousePos.x != last_mouse_pos.x || io.MousePos.y != last_mouse_pos.y);
        has_mouse_delta = has_mouse_delta || (io.MouseDelta.x != 0 || io.MouseDelta.y != 0);

        /*if(can_suppress_inputs)
        {
            has_mouse_delta = false;
        }*/

        //any_events = any_events || has_mouse_delta;
        visual_events = visual_events || (io.MouseWheel != 0 || io.MouseWheelH != 0);
        visual_events = visual_events || (io.DisplaySize.x != last_display_size.x || io.DisplaySize.y != last_display_size.y);
        visual_events = visual_events || to_read.websocket_read_queue[-1].size() > 0 || to_read.new_clients.size() > 0;
        //any_events = any_events || (last_can_suppress_inputs != can_suppress_inputs);
        //any_events = any_events || (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered());

        //any_events = any_events || (unprocessed_frames >= max_unprocessed_frames);

        visual_events = visual_events || window.has_dropped_file();
        bool any_active = (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered());

        //std::cout << "Hovered? " << any_active << std::endl;

        visual_events = visual_events || any_active;
        last_item_active = any_active;
        visual_events = visual_events || (last_item_active != any_active);

        if(s_api.enabled && s_api.is_overlay_open())
            visual_events = visual_events || (sleep_limiter.get_elapsed_time_s() > max_sleep);

        bool non_visual_events = has_mouse_delta;

        //printf("Any events? %i %f %f\n", any_events, io.MousePos.x, io.MousePos.y);

        last_display_size = io.DisplaySize;
        last_mouse_pos = io.MousePos;
        //last_can_suppress_inputs = can_suppress_inputs;

        if(!terminals.all_cache_valid())
            visual_events = true;

        for(auto& i : chat_win.chat_threads)
        {
            if(i.second.was_rendered && !i.second.cache.valid())
                visual_events = true;
        }

        if(last_is_open != s_api.is_overlay_open())
        {
            visual_events = true;
        }

        #ifdef NO_SLEEP
        visual_events = true;
        non_visual_events = true;
        #endif // NO_SLEEP

        //#ifdef __EMSCRIPTEN__
        if(visual_events || non_visual_events)
        //#endif // __EMSCRIPTEN__
        {
            if(visual_events)
            {
                sleep_limiter.restart();
            }

            if(visual_events)
            {
                terminals.invalidate_visual_cache();

                for(auto& i : chat_win.chat_threads)
                {
                    i.second.cache.invalidate_visual_cache();
                }

                for(auto& i : realtime_scripts.windows)
                {
                    i.second.cache.invalidate_visual_cache();
                }
            }

            if(font_select.update_rebuild())
            {
                invalidate_everything(terminals, chat_win);

                ImGui_ImplOpenGL3_DestroyDeviceObjects();
                ImGui_ImplOpenGL3_CreateDeviceObjects();

                unsigned char* pixels = nullptr;
                ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, nullptr, nullptr, nullptr);
            }

            clipboard::poll();

            //#ifdef __EMSCRIPTEN__
            window.poll_issue_new_frame_only();
            //#endif // __EMSCRIPTEN__

            if(window.has_dropped_file())
            {
                dropped_file drop = window.get_next_dropped_file();

                if(!is_key_file(drop.name))
                {
                    ImGui::Begin("File Upload Dialogue", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

                    ImGui::TextUnformatted(drop.name.c_str());

                    ImGui::SameLine();

                    if(ImGui::Button("Upload"))
                    {
                        std::vector<std::string> post_split = no_ss_split(drop.name, ".");

                        std::cout << "DROP NAME " << drop.name << " post split " << post_split.size() << std::endl;

                        std::cout << "PSBACK " << post_split.back().size() << "|HI" << std::endl;

                        ///i20k.scriptname.js or scriptname.js
                        if((post_split.size() == 2 || post_split.size() == 3) && post_split.back() == "js")
                        {
                            std::string name = post_split.size() == 2 ? post_split[0] : post_split[1];

                            if(is_valid_full_name_string(current_user + "." + name))
                            {
                                std::string fstr = "#up_es6 " + name + " " + drop.data;

                                nlohmann::json data;
                                data["type"] = "generic_server_command";
                                data["data"] = fstr;

                                write_data dat;
                                dat.id = -1;
                                dat.data = data.dump();

                                to_write.write_to_websocket(std::move(dat));
                            }
                        }

                        window.pop_dropped_file();
                    }

                    ImGui::SameLine();

                    if(ImGui::Button("Cancel"))
                    {
                        window.pop_dropped_file();
                    }

                    ImGui::End();
                }
                else
                {
                    nlohmann::json data;
                    data["type"] = "key_auth";
                    data["data"] = drop.data;

                    write_data dat;
                    dat.id = -1;
                    dat.data = data.dump();

                    to_write.write_to_websocket(std::move(dat));

                    bool should_save = !file::exists("hex_key.key");

                    if(should_save)
                    {
                        file::write_atomic("hex_key.key", drop.data, file::mode::BINARY);
                    }

                    window.pop_dropped_file();
                }
            }

            unprocessed_frames = 0;

            /*if(font_select.update_rebuild(window, font_select.current_base_font_size))
            {
                term.invalidate();
            }*/

            vec2f cursor_pos = {io.MousePos.x, io.MousePos.y};

            for(int i=0; i < (int)(sizeof(curKeysDown) / sizeof(curKeysDown[0])); i++)
            {
                ///this isn't working correctly on emscripten, probably due to the callback issue
                if((curKeysDown[i] && !lastKeysDown[i]) || ImGui::IsKeyPressed(i))
                {
                    glfw_key_pressed_data.push_back(i);
                }

                if(!curKeysDown[i] && lastKeysDown[i])
                {
                    glfw_key_released_data.push_back(i);
                }
            }

            for(int i=0; i < (int)(sizeof(curMouseDown) / sizeof(curMouseDown[0])); i++)
            {
                if(curMouseDown[i] && !lastMouseDown[i])
                {
                    mouse_pressed_data.push_back(i);
                }

                if(!curMouseDown[i] && lastMouseDown[i])
                {
                    mouse_released_data.push_back(i);
                }
            }

            float scroll_y = io.MouseWheel;
            //float scroll_x = io.MouseWheelH;

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

                    last_line_invalidate_everything(terminals, chat_win);

                    realtime_str.push_back(utf8);
                }
            }

            for(int i : glfw_key_pressed_data)
            {
                if(auto val = backend->get_key_name(i); val != "")
                    on_pressed.push_back(val);

                last_line_invalidate_everything(terminals, chat_win);

                if(i == io.KeyMap[ImGuiKey_Backspace])
                {
                    if(io.KeyCtrl)
                    {
                        for(int kk=0; kk < 5; kk++)
                            to_edit->process_backspace();
                    }
                    else
                    {
                        to_edit->process_backspace();
                    }
                }

                if(i == io.KeyMap[ImGuiKey_Delete])
                {
                    if(io.KeyCtrl)
                    {
                        for(int kk=0; kk < 5; kk++)
                            to_edit->process_delete();
                    }
                    else
                    {
                        to_edit->process_delete();
                    }
                }

                if(i == io.KeyMap[ImGuiKey_UpArrow])
                {
                    to_edit->move_command_history_idx(-1);
                }

                if(i == io.KeyMap[ImGuiKey_DownArrow])
                {
                    to_edit->move_command_history_idx(1);
                }

                if(i == io.KeyMap[ImGuiKey_LeftArrow])
                {
                    if(!io.KeyCtrl)
                        to_edit->move_cursor(-1);
                    else
                        to_edit->move_cursor(-5);
                }

                if(i == io.KeyMap[ImGuiKey_RightArrow])
                {
                    if(!io.KeyCtrl)
                        to_edit->move_cursor(1);
                    else
                        to_edit->move_cursor(5);
                }

                if(i == io.KeyMap[ImGuiKey_Home])
                {
                    to_edit->move_cursor(-(int)to_edit->command.size());
                }

                if(i == io.KeyMap[ImGuiKey_End])
                {
                    to_edit->move_cursor(to_edit->command.size());
                }

                if(i == io.KeyMap[ImGuiKey_Escape])
                {
                    to_edit->clear_command();
                }

                if(i == io.KeyMap[ImGuiKey_V])
                {
                    if(io.KeyCtrl)
                    {
                        std::string add_text = clipboard::get();

                        for(auto& c : add_text)
                        {
                            to_edit->add_to_command(c);
                        }
                    }
                }

                if(i == GLFW_KEY_F1)
                {
                    font_select.is_open = !font_select.is_open;
                }

                if(i == io.KeyMap[ImGuiKey_Enter] || i == io.KeyMap[ImGuiKey_KeyPadEnter])
                {
                    if(to_edit != &realtime_shim && to_edit != &no_string)
                    {
                        if(!io.KeyCtrl && !io.KeyShift)
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
                if(auto val = backend->get_key_name(i); val != "")
                    on_released.push_back(val);
            }

            for(int i : mouse_pressed_data)
            {
                if(mouse_map.find(i) != mouse_map.end())
                    on_pressed.push_back(mouse_map[i]);

                if(i == 1)
                {
                    std::string add_text = clipboard::get();

                    for(auto& c : add_text)
                    {
                        to_edit->add_to_command(c);
                    }

                    last_line_invalidate_everything(terminals, chat_win);
                }

                if(i == 0)
                {
                    last_line_invalidate_everything(terminals, chat_win);

                    get_global_copy_handler()->on_lclick(cursor_pos);
                }
            }

            for(int i : mouse_released_data)
            {
                if(mouse_map.find(i) != mouse_map.end())
                    on_released.push_back(mouse_map[i]);

                if(i == 0)
                {
                    last_line_invalidate_everything(terminals, chat_win);

                    get_global_copy_handler()->on_lclick_release(cursor_pos);
                }
            }

            mouse_delta += scroll_y;
            script_mousewheel_delta += scroll_y;

            ///todo: this isn't good enough. We need input *text*, and the stream of pressed keys to be presented to the script
            ///https://love2d.org/wiki/love.textinput
            ///https://love2d.org/wiki/love.keypressed
            if(realtime_scripts.get_id_of_focused_realtime_window() != -1 && (realtime_str.size() > 0 || on_pressed.size() > 0 || on_released.size() > 0))
            {
                nlohmann::json data;
                data["type"] = "send_keystrokes_to_script";
                data["id"] = realtime_scripts.get_id_of_focused_realtime_window();
                data["input_keys"] = realtime_str;
                data["pressed_keys"] = on_pressed;
                data["released_keys"] = on_released;

                write_data dat;
                dat.id = -1;
                dat.data = data.dump();

                to_write.write_to_websocket(std::move(dat));
            }

            if(realtime_scripts.get_id_of_focused_realtime_window() != -1 && mouse_clock.get_elapsed_time_s() * 1000 >= mouse_send_time_ms)
            {
                mouse_clock.restart();

                vec2f mpos = cursor_pos;

                realtime_script_run& run = realtime_scripts.windows[realtime_scripts.get_id_of_focused_realtime_window()];

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
                    data["id"] = realtime_scripts.get_id_of_focused_realtime_window();
                    data["mouse_x"] = char_mpos.x();
                    data["mouse_y"] = char_mpos.y();
                    data["mousewheel_x"] = 0.f;
                    data["mousewheel_y"] = script_mousewheel_delta;

                    write_data dat;
                    dat.id = -1;
                    dat.data = data.dump();

                    to_write.write_to_websocket(std::move(dat));
                }

                script_mousewheel_delta = 0;
            }

            if(realtime_scripts.get_id_of_focused_realtime_window() == -1)
                script_mousewheel_delta = 0;

            terminals.main_terminal.scroll_hack.scrolled_this_frame = mouse_delta;

            for(auto& i : terminals.sub_terminals)
            {
                i.second.scroll_hack.scrolled_this_frame = mouse_delta;
            }

            for(auto& i : chat_win.chat_threads)
            {
                i.second.scroll_hack.scrolled_this_frame = mouse_delta;
            }

            for(auto& i : realtime_scripts.windows)
            {
                i.second.scroll_hack.scrolled_this_frame = mouse_delta;
            }

            ImGui::PushFont(font_select.get_base_font());

            auth_manage.display(terminals, s_api, to_write, current_user);

            font_select.render(window);

            /*if(window_ctx.srgb_dirty)
            {
                ImGui::SetStyleLinearColor(window_ctx.is_srgb);
            }*/

            terminals.main_terminal.check_insert_user_command();

            if(starts_with(to_edit->command, "user user ") && terminals.get_focused_terminal()->focused)
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
                terminal_imgui& term = *terminals.get_focused_terminal();

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
                        std::string up_data = default_up_handling(current_user, term.command.command, get_scripts_directory(current_user) + "/");

                        nlohmann::json data;
                        data["type"] = "generic_server_command";
                        data["data"] = up_data;

                        if(terminals.get_focused_terminal_id() > 0)
                        {
                            data["tag"] = terminals.get_focused_terminal_id();
                        }

                        write_data dat;
                        dat.id = -1;
                        dat.data = data.dump();

                        to_write.write_to_websocket(std::move(dat));
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
                        term.add_text("Syntax is /join channel password", terminals.auto_handle);
                        bump = true;
                    }
                    else if(command == "/leave")
                    {
                        term.add_text("Syntax is /leave channel", terminals.auto_handle);
                        bump = true;
                    }
                    else if(command == "/create")
                    {
                        term.add_text("Syntax is /create channel password", terminals.auto_handle);
                        bump = true;
                    }
                    else if(starts_with(command, "/"))
                    {
                        bump = true;

                        int idx = 0;

                        for(; idx < (int)command.size() && command[idx] != ' '; idx++);

                        if(idx + 1 >= (int)command.size())
                        {
                            term.add_text("First argument must be a channel name, eg /join global", terminals.auto_handle);
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
                                term.add_text("Not a valid command, try /join, /leave or /create", terminals.auto_handle);
                            }

                            if(final_command != "")
                            {
                                nlohmann::json data;
                                data["type"] = "client_chat";
                                data["respond"] = 1;
                                data["data"] = final_command;

                                write_data dat;
                                dat.id = -1;
                                dat.data = data.dump();

                                to_write.write_to_websocket(std::move(dat));
                            }
                        }
                    }
                    else if(has_chat_window)
                    {
                        std::optional<chat_thread*> focused_thread = chat_win.get_focused_chat_thread();

                        if(focused_thread.has_value())
                        {
                            std::string escaped_string = escape_str(focused_thread.value()->command.command);

                            nlohmann::json data;
                            data["type"] = "client_chat";
                            data["data"] = "#hs.msg.send({channel:\"" + focused_thread.value()->name + "\", msg:\"" + escaped_string + "\"})";

                            write_data dat;
                            dat.id = -1;
                            dat.data = data.dump();

                            to_write.write_to_websocket(std::move(dat));
                        }
                    }

                    if(bump)
                    {
                        chat_win.add_text_to_focused(command);

                        std::optional<chat_thread*> focused_thread = chat_win.get_focused_chat_thread();

                        if(focused_thread)
                        {
                            focused_thread.value()->cache.invalidate();
                        }
                    }
                }

                std::string cmd = term.command.command;

                if(term.focused)
                {
                    term.bump_command_to_history(terminals.auto_handle);
                }
                else if(auto opt = chat_win.get_focused_chat_thread(); opt.has_value())
                {
                    opt.value()->command.clear_command();
                }

                if(term.focused && is_local_command(cmd))
                {
                    bool should_shutdown = false;

                    std::string data = handle_local_command(current_user, cmd, terminals.auto_handle, should_shutdown, terminals, chat_win);

                    term.add_text(data, terminals.auto_handle);

                    if(should_shutdown)
                    {
                        window.close();
                    }
                }

                pretty_atomic_write_all(terminal_file, serialise(terminals, serialise_mode::DISK));
                pretty_atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK));
            }
            else if(enter && to_edit->command.size() == 0)
            {
                terminals.get_focused_terminal()->add_text(" ", terminals.auto_handle);
            }

            if(ImGui::IsMouseDown(0))
            {
                get_global_copy_handler()->on_hold_lclick(cursor_pos);
            }

            for(write_data& dat : to_read.websocket_read_queue[-1])
            {
                #ifdef TESTING
                api_calls.push_back(dat.data);
                #endif // TESTING

                nlohmann::json data;

                try
                {
                    data = nlohmann::json::parse(dat.data);
                }
                catch(...)
                {
                    std::cout << "Error Data Str " << dat.data << std::endl;
                }

                process_text_from_server(terminals, auth_manage, current_user, data, chat_win, font_select, realtime_scripts);
            }

            if(write_clock.get_elapsed_time_s() > 5 && (!terminals.all_cache_valid() || chat_win.any_cache_invalid()))
            {
                pretty_atomic_write_all(terminal_file, serialise(terminals, serialise_mode::DISK));
                pretty_atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK));

                auto save_sett = window.get_render_settings();

                pretty_atomic_write_all(window_file, serialise(save_sett, serialise_mode::DISK));

                write_clock.restart();
            }

            ///hmm
            ///this is inadequate
            ///we need to be able to request multiple scripts at once
            ///and receive multiple as well
            if(terminals.auto_handle.found_unprocessed_autocompletes.size() > 0 && request_clock.get_elapsed_time_s() > 0.3)
            {
                request_clock.restart();

                for(const std::string& str : terminals.auto_handle.found_unprocessed_autocompletes)
                {
                    nlohmann::json data;
                    data["type"] = "autocomplete_request";
                    data["data"] = str;

                    write_data dat;
                    dat.id = -1;
                    dat.data = data.dump();

                    to_write.write_to_websocket(std::move(dat));

                    break;
                }

                if(terminals.auto_handle.found_unprocessed_autocompletes.size() > 0)
                    terminals.auto_handle.found_unprocessed_autocompletes.erase(terminals.auto_handle.found_unprocessed_autocompletes.begin());
            }

            if((terminals.get_focused_terminal()->focused || realtime_scripts.get_id_of_focused_realtime_window() != 1) && io.KeyCtrl && just_pressed(io.KeyMap[ImGuiKey_C]))
            {
                nlohmann::json data;
                data["type"] = "client_terminate_scripts";
                data["id"] = -1;

                write_data dat;
                dat.id = -1;
                dat.data = data.dump();

                to_write.write_to_websocket(std::move(dat));
            }

            if(has_settings_window)
            {
                ImGui::Begin("Settings", &has_settings_window);

                ImGui::Checkbox("Show chat in terminal", &chat_win.show_chat_in_main_window);

                if(ImGui::Button("New terminal"))
                {
                    terminals.make_new_terminal();
                }

                ImGui::End();
            }

            {
                ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

                ImGui::Begin("notepad");

                bool changed = ImGui::InputTextMultiline("Doot", &notepad, ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_None);

                if(changed)
                {
                    file::write(notepad_file, notepad, file::mode::BINARY);
                }

                ImGui::End();
            }

            if(ImGui::IsKeyPressed(GLFW_KEY_F3))
                has_settings_window = !has_settings_window;

            //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

            int was_closed_id = -1;

            vec2i window_dim = window.get_window_size();

            //test_imgui_term.render(window);
            realtime_scripts.render_realtime_windows(to_write, was_closed_id, font_select, terminals.auto_handle, window.get_render_settings().is_srgb);
            chat_win.render(should_coordinate_focus);
            terminals.render(window, {window_dim.x(), window_dim.y()}, should_coordinate_focus);

            should_coordinate_focus = false;

            if(was_closed_id != -1)
            {
                nlohmann::json data;
                data["type"] = "client_terminate_scripts";
                data["id"] = was_closed_id;

                write_data dat;
                dat.id = -1;
                dat.data = data.dump();

                to_write.write_to_websocket(std::move(dat));
            }

            if(terminals.auto_handle.tab_pressed)
            {
                last_line_invalidate_everything(terminals, chat_win);
            }

            terminals.auto_handle.tab_pressed = ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Tab]);

            if(terminals.auto_handle.tab_pressed)
            {
                last_line_invalidate_everything(terminals, chat_win);
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
                invalidate_everything(terminals, chat_win);
            }

            ImGui::PopFont();

            if(visual_events)
                window.display();

            if(!visual_events)
            {
                ImGui::Render();

                if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    ImGui::UpdatePlatformWindows();
                }
            }
        }
        //#ifdef __EMSCRIPTEN__
        else
        {
            #ifndef __EMSCRIPTEN__
            /*double extra = max_sleep - slept_for;

            if(extra > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)round(extra * 1000)));
            }*/

            #endif // __EMSCRIPTEN__

            unprocessed_frames++;
            //window.display_last_frame();
        }
        //#endif

        conn.send_bulk(to_write);
    };

    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg((em_arg_callback_func)main_loop_helper, nullptr, 0, 1);
    #endif

    /*#ifdef __EMSCRIPTEN__
    emscripten::val::global("window").call<void>(
    "handle_download",
    std::string("filename.ext")
    );
    #endif // __EMSCRIPTEN__*/

    file::write(notepad_file, notepad, file::mode::BINARY);
    pretty_atomic_write_all(terminal_file, serialise(terminals, serialise_mode::DISK));
    pretty_atomic_write_all(chat_file, serialise(chat_win, serialise_mode::DISK));

    auto save_sett = window.get_render_settings();

    pretty_atomic_write_all(window_file, serialise(save_sett, serialise_mode::DISK));

    return 0;
}
