#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_opengl3.h>
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

#include <nlohmann/json.hpp>
#include <stdio.h>
#include "font_cfg.hpp"
#include <iomanip>
#include <misc/cpp/imgui_stdlib.h>
#include <toolkit/fs_helpers.hpp>

#ifdef CUSTOM_PROFILING
#include "custom_profile.hpp"
#endif // CUSTOM_PROFILING

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#endif // __EMSCRIPTEN__

#include "render_string.hpp"
#include "script_transfer.hpp"
#include "context.hpp"

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
#define HOST_IP "77.98.166.61"
//#define HOST_IP "netcodegame.xyz"
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

    //test_render_strings();

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
    conn.connect(connection_ip, connection_port, connection_type::SSL, "netcodegame.xyz");
    #else
    conn.set_client_sleep_interval(8);
    conn.connect(connection_ip, connection_port, connection_type::EMSCRIPTEN_AUTOMATIC);
    #endif

    #ifdef CUSTOM_PROFILING
    profiling::disable_thread(conn.thrd.back().get_id());
    #endif // CUSTOM_PROFILING

    connection_send_data to_write(conn.get_settings());
    connection_received_data to_read;

    printf("Post Connect\n");

    render_settings sett;

    auth_manager auth_manage;

    auth_manage.check(s_api, to_write, "");
    conn.send_bulk(to_write);

    bool has_file = false;

    std::string window_file = "window_v3.txt";

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

    ImGuiIO& io = ImGui::GetIO();

    io.MouseDragThreshold = 0;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
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

    context ctx;

    ctx.backend = backend;

    auto_handler test_handler;
    chat_manager chat2;
    realtime_script_manager2 realtime_scripts2;

    font_selector& font_select = ctx.font_select;
    font_select.reset_default_fonts();

    printf("Terminal + Chat window\n");

    printf("Specials\n");

    std::string terminal_file = "terminal_v8.txt";
    std::string chat_file = "chat_v7.txt";
    //std::string settings_file = "text_sett_v1.txt";
    std::string font_file = "font_sett_v2.txt";
    std::string autos_file = "autocompletes_v1.txt";
    std::string notepad_file = "notepad.txt";

    try
    {
        if(file::exists(terminal_file))
        {
            nlohmann::json dat = nlohmann::json::parse(file::read(terminal_file, file::mode::BINARY));
            deserialise(dat, ctx.terminals, serialise_mode::DISK);
        }
    }
    catch(...){printf("Invalid terminal file\n");}

    try
    {
        if(file::exists(chat_file))
        {
            nlohmann::json dat = nlohmann::json::parse(file::read(chat_file, file::mode::BINARY));
            deserialise(dat, chat2, serialise_mode::DISK);
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

    try
    {
        if(file::exists(autos_file))
        {
            nlohmann::json dat = nlohmann::json::parse(file::read(autos_file, file::mode::BINARY));
            deserialise(dat, test_handler, serialise_mode::DISK);
        }
    }
    catch(...){printf("Invalid autocompletes file\n");}

    std::string notepad;

    if(file::exists(notepad_file))
    {
        notepad = file::read(notepad_file, file::mode::BINARY);
    }

    printf("Loaded files\n");

    steady_timer render_clock;
    steady_timer write_clock;

    #ifdef TESTING
    std::vector<std::string> api_calls;
    #endif // TESTING

    steady_timer connection_clock;

    bool lastKeysDown[512] = {};
    bool lastMouseDown[5] = {};
    bool curKeysDown[512] = {};
    bool curMouseDown[5] = {};
    ImVec2 last_mouse_pos = ImVec2(0,0);
    ImVec2 last_display_size = ImVec2(0,0);

    bool has_settings_window = false;

    printf("Pre main loop\n");

    int iconn = conn.client_connected_to_server;
    printf("Am connected? %i\n", iconn);

    int max_unprocessed_frames = 200;
    int unprocessed_frames = max_unprocessed_frames;
    bool last_item_active = false;

    steady_timer sleep_limiter;

    bool printed_connecting = false;

    //while(running)
    #ifndef __EMSCRIPTEN__
    while(!window.should_close() && ctx.terminals.primary.open)
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
            conn.connect(connection_ip, connection_port, connection_type::SSL, "netcodegame.xyz");
            #else
            conn.connect(connection_ip, connection_port, connection_type::EMSCRIPTEN_AUTOMATIC);
            #endif

            #ifdef CUSTOM_PROFILING
            profiling::disable_thread(conn.thrd.back().get_id());
            #endif // CUSTOM_PROFILING

            connection_clock.restart();

            auth_manage.check(s_api, to_write, ctx.root_user);

            if(!printed_connecting)
                ctx.terminals.primary.add_main_text("Connecting...");
            else
                ctx.terminals.primary.add_main_text(".");

            printed_connecting = true;
        }

        if(conn.client_connected_to_server)
        {
            printed_connecting = false;
        }

        bool last_is_open = s_api.is_overlay_open();

        s_api.pump_callbacks();

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

            if(font_select.update_rebuild())
            {
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

                            if(is_valid_full_name_string(ctx.root_user + "." + name))
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

            vec2f cursor_pos = {io.MousePos.x, io.MousePos.y};

            if(ImGui::IsMouseClicked(0))
            {
                get_global_copy_handler2().on_lclick(cursor_pos);
            }

            if(ImGui::IsMouseReleased(0))
            {
                get_global_copy_handler2().on_lclick_release(cursor_pos);
            }

            if(!ImGui::IsMouseDown(0))
            {
                get_global_copy_handler2().on_no_lclick();
            }

            ///todo: this isn't good enough. We need input *text*, and the stream of pressed keys to be presented to the script
            ///https://love2d.org/wiki/love.textinput
            ///https://love2d.org/wiki/love.keypressed

            ImGui::PushFont(font_select.get_base_font());

            auth_manage.display(ctx.terminals.primary, s_api, to_write, ctx.root_user);

            font_select.render(window);

            if(ImGui::IsMouseDown(0))
            {
                get_global_copy_handler2().on_hold_lclick(cursor_pos);
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

                filter_down_handling(ctx.terminals.primary, data);

                ctx.terminals.extract_server_commands(ctx, data, test_handler);
                realtime_scripts2.extract_server_commands(font_select, data);
                test_handler.extract_server_commands(data);
                chat2.extract_server_commands(data);
                auth_manage.extract_server_commands(ctx.terminals.primary, data);
            }

            if(write_clock.get_elapsed_time_s() > 5)
            {
                pretty_atomic_write_all(terminal_file, serialise(ctx.terminals, serialise_mode::DISK));
                pretty_atomic_write_all(chat_file, serialise(chat2, serialise_mode::DISK));
                pretty_atomic_write_all(autos_file, serialise(test_handler, serialise_mode::DISK));

                auto save_sett = window.get_render_settings();

                pretty_atomic_write_all(window_file, serialise(save_sett, serialise_mode::DISK));

                write_clock.restart();
            }

            ///hmm
            ///this is inadequate
            ///we need to be able to request multiple scripts at once
            ///and receive multiple as well
            test_handler.make_server_request(to_write);

            if(has_settings_window)
            {
                ImGui::Begin("Settings", &has_settings_window);

                ImGui::Checkbox("Show chat in terminal", &ctx.show_chat_in_main_window);

                if(ImGui::Button("New terminal"))
                {
                    ctx.terminals.create_new_terminal();
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

            realtime_scripts2.default_controls(ctx, test_handler, to_write);
            realtime_scripts2.render(ctx, test_handler, to_write);

            ctx.terminals.default_controls(ctx, test_handler, to_write);
            ctx.terminals.render(ctx, test_handler, to_write);

            chat2.default_controls(ctx, test_handler, to_write);
            chat2.render(ctx, test_handler, to_write);

            char_inf::cwidth = ImGui::CalcTextSize("A").x + char_inf::extra_glyph_spacing;
            char_inf::cheight = ImGui::CalcTextSize("A").y;

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

        if(ctx.should_shutdown)
        {
            window.close();
        }
    };

    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg((em_arg_callback_func)main_loop_helper, nullptr, 0, 1);
    #endif

    #ifdef CUSTOM_PROFILING
    std::cout << "TARGETED " << profiling::format_profiling_data("string_to_interop") << std::endl;

    std::string str = profiling::format_profiling_data();

    std::cout << str << std::endl;

    file::write("prof.txt", str, file::mode::TEXT);
    #endif // CUSTOM_PROFILING

    /*#ifdef __EMSCRIPTEN__
    emscripten::val::global("window").call<void>(
    "handle_download",
    std::string("filename.ext")
    );
    #endif // __EMSCRIPTEN__*/

    file::write(notepad_file, notepad, file::mode::BINARY);
    pretty_atomic_write_all(terminal_file, serialise(ctx.terminals, serialise_mode::DISK));
    pretty_atomic_write_all(chat_file, serialise(chat2, serialise_mode::DISK));

    auto save_sett = window.get_render_settings();

    pretty_atomic_write_all(window_file, serialise(save_sett, serialise_mode::DISK));

    return 0;
}
