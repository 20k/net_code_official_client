#ifndef IMGUI_UI_COMPONENTS_HPP_INCLUDED
#define IMGUI_UI_COMPONENTS_HPP_INCLUDED

#include <serialise/serialise.hpp>
#include "colour_interop.hpp"
#include "editable_string.hpp"
#include "auto_handlers.hpp"

struct chat_thread;
struct chat_window;

struct scrollbar_hack
{
    ///lines above bottom
    float scrolled = 0;
    float scrolled_this_frame = 0.f;

    float output_scroll_frac = 0.f;

    bool scrolling = false;
    bool should_update_scroll = false;

    void do_hack(sf::RenderWindow& win, int approx_num, bool set_scrollbar);
};

struct terminal_imgui : serialisable
{
    scrollbar_hack scroll_hack;
    std::vector<interop_vec_t> text_history;

    std::map<std::string, chat_thread> chat_threads;

    bool consider_resetting_scrollbar = false;
    bool locked_to_bottom = false;
    bool lock_next_frame = true;
    bool focused = true;
    editable_string command;

    auto_handler auto_handle;

    virtual void do_serialise(serialise& s, bool ser);

    void clear_terminal();
    void clear_chat();
    void clear_text();

    terminal_imgui();
    void render(sf::RenderWindow& win);
    void bump_command_to_history();

    void add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command = true);
};

#endif // IMGUI_UI_COMPONENTS_HPP_INCLUDED
