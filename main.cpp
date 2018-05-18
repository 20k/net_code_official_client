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
#include <imgui/imgui_internal.h>
#include <imgui-sfml/imgui-SFML.h>
#include <imgui/misc/freetype/imgui_freetype.h>
#include "imgui_ui_components.hpp"

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

void update_font_texture_safe()
{
    static sf::Texture texture;

    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    texture.create(width, height);
    texture.update(pixels);

    io.Fonts->TexID = reinterpret_cast<void*>(texture.getNativeHandle());
}

struct font_selector
{
    bool wants_rebuild = true;
    float fonts_multiply = 1.f;
    unsigned int fonts_flags = ImGuiFreeType::ForceAutoHint | ImGuiFreeType::MonoHinting;

    bool is_open = false;

    // Call _BEFORE_ NewFrame()
    bool update_rebuild()
    {
        if (!wants_rebuild)
            return false;

        ImGuiIO& io = ImGui::GetIO();

        for (int n = 0; n < io.Fonts->Fonts.Size; n++)
        {
            //io.Fonts->Fonts[n]->ConfigData->RasterizerMultiply = FontsMultiply;
            //io.Fonts->Fonts[n]->ConfigData->RasterizerFlags = (BuildMode == FontBuildMode_FreeType) ? fonts_flags : 0x00;
        }

        ImGuiFreeType::BuildFontAtlas(io.Fonts, fonts_flags);

        wants_rebuild = false;
        return true;
    }

    // Call to draw interface
    void render()
    {
        if(!is_open)
            return;

        ImGui::Begin("FreeType Options", &is_open);
        ImGui::ShowFontSelector("Fonts");

        wants_rebuild |= ImGui::DragFloat("Multiply", &fonts_multiply, 0.001f, 0.0f, 2.0f);

        wants_rebuild |= ImGui::CheckboxFlags("NoHinting",     &fonts_flags, ImGuiFreeType::NoHinting);
        wants_rebuild |= ImGui::CheckboxFlags("NoAutoHint",    &fonts_flags, ImGuiFreeType::NoAutoHint);
        wants_rebuild |= ImGui::CheckboxFlags("ForceAutoHint", &fonts_flags, ImGuiFreeType::ForceAutoHint);
        wants_rebuild |= ImGui::CheckboxFlags("LightHinting",  &fonts_flags, ImGuiFreeType::LightHinting);
        wants_rebuild |= ImGui::CheckboxFlags("MonoHinting",   &fonts_flags, ImGuiFreeType::MonoHinting);
        wants_rebuild |= ImGui::CheckboxFlags("Bold",          &fonts_flags, ImGuiFreeType::Bold);
        wants_rebuild |= ImGui::CheckboxFlags("Oblique",       &fonts_flags, ImGuiFreeType::Oblique);

        ImGui::End();
    }
};

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

    token_tests();

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
    window.create(sf::VideoMode(1200,600), "net_code", sf::Style::Default, sett);

    ImGui::SFML::Init(window, false);

    ImFontConfig font_cfg;
    font_cfg.GlyphExtraSpacing = ImVec2(char_inf::extra_glyph_spacing, 0);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", 14.f, &font_cfg);
    io.Fonts->AddFontDefault();

    //ImGuiFreeType::BuildFontAtlas(io.Fonts, 0);
    //ImGui::SFML::UpdateFontTexture();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(30/255.f, 30/255.f, 30/255.f, 255/255.f));

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 2;
    style.WindowRounding = 2;
    style.ChildRounding = 2;
    style.ChildBorderSize = 0;
    style.FrameBorderSize = 0;
    style.PopupBorderSize = 0;
    style.WindowBorderSize = 0;

    /*ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", 13.f);
    io.Fonts->AddFontDefault();*/

    font_selector font_select;

    terminal_imgui term;
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

    std::string terminal_file = "./terminal_v5.txt";
    std::string chat_file = "./chat_v5.txt";

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

    sf::Clock imgui_delta;

    #ifdef TESTING
    std::vector<std::string> api_calls;
    #endif // TESTING

    //double diff_s = 0.f;

    editable_string realtime_shim;

    bool running = true;

    while(running)
    {
        if(font_select.update_rebuild())
        {
            update_font_texture_safe();
        }

        realtime_shim.clear_command();

        editable_string* to_edit = &term.command;

        if(chat_win.focused)
            to_edit = &chat_win.command;

        if(term.get_id_of_focused_realtime_window() != -1)
            to_edit = &realtime_shim;

        bool enter = false;

        float mouse_delta = 0.f;

        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);

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
                }

                if(event.key.code == sf::Keyboard::Down)
                {
                    to_edit->move_command_history_idx(1);
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

                if(event.key.code == sf::Keyboard::F1)
                {
                    font_select.is_open = !font_select.is_open;
                }

                if(event.key.code == sf::Keyboard::Return)
                {
                    if(to_edit != &realtime_shim)
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

            if(event.type == sf::Event::MouseWheelScrolled)
            {
                mouse_delta += event.mouseWheelScroll.delta;
            }
        }

        if(term.get_id_of_focused_realtime_window() != -1 && to_edit->command.size() > 0)
        {
            ///pipe keys to server
            ///todo make enter work

            sa_do_send_keystrokes_to_script(shared, term.get_id_of_focused_realtime_window(), make_view(to_edit->command));
            to_edit->clear_command();
        }

        term.scroll_hack.scrolled_this_frame = mouse_delta;
        chat_win.scroll_hack.scrolled_this_frame = mouse_delta;

        ImGui::SFML::Update(window,  imgui_delta.restart());

        //ImGui::ShowDemoWindow(nullptr);

        font_select.render();

        if(enter)
        {
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
                sized_string chat_command = sa_make_chat_command(make_view(chat_win.selected), make_view(chat_win.command.command));

                ///TODO
                sd_add_back_write(shared, make_view(chat_command));

                free_sized_string(chat_command);
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

        if(mouse.isButtonPressed(sf::Mouse::Left) && is_focused(window))
            get_global_copy_handler()->on_hold_lclick(window,  vpos);

        if(sd_has_front_read(shared))
        {
            sized_string c_data = sd_get_front_read(shared);
            std::string fdata = c_str_sized_to_cpp(c_data);
            free_sized_string(c_data);

            #ifdef TESTING
            api_calls.push_back(fdata);
            #endif // TESTING

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
                sa_do_autocomplete_request(shared, make_view(str));

                break;
            }

            if(term.auto_handle.found_unprocessed_autocompletes.size() > 0)
                term.auto_handle.found_unprocessed_autocompletes.erase(term.auto_handle.found_unprocessed_autocompletes.begin());

            //term.auto_handle.found_unprocessed_autocompletes.clear();
        }

        if(term.focused && key.isKeyPressed(sf::Keyboard::LControl) && ONCE_MACRO(sf::Keyboard::C))
        {
            sa_do_terminate_script(shared);
        }

        //std::cout << render_clock.restart().asMicroseconds() / 1000.f << std::endl;

        term.auto_handle.window_in_focus = is_focused(window);
        chat_win.auto_handle.window_in_focus = is_focused(window);

        //test_imgui_term.text_history = term.text_history;

        //test_imgui_term.render(window);
        term.render(window);
        term.render_realtime_windows();
        chat_win.render(window, term.chat_threads);

        term.auto_handle.tab_pressed = ONCE_MACRO(sf::Keyboard::Tab) && is_focused(window);

        ///this is a hack to fix the fact that sometimes
        ///click input doesn't make clean click/release pairs
        if(!mouse.isButtonPressed(sf::Mouse::Left))
        {
            get_global_copy_handler()->finished = false;
            get_global_copy_handler()->held = false;
        }

        ImGui::SFML::Render(window);
        window.display();
        window.clear(sf::Color(30, 30, 30));

        sf::sleep(sf::milliseconds(4));

        char_inf::cwidth = ImGui::CalcTextSize("A").x + char_inf::extra_glyph_spacing;
        char_inf::cheight = ImGui::CalcTextSize("A").y;
    }

    serialise sterm;
    sterm.handle_serialise(term, true);
    sterm.save(terminal_file);

    serialise swindow;
    swindow.handle_serialise(chat_win, true);
    swindow.save(chat_file);

    sd_set_termination(shared);

    sf::sleep(sf::milliseconds(100));

    CoUninitialize();

    //while(shared.termination_count != 3) {}

    return 0;
}
