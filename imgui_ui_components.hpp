#ifndef IMGUI_UI_COMPONENTS_HPP_INCLUDED
#define IMGUI_UI_COMPONENTS_HPP_INCLUDED

#include <serialise/serialise.hpp>
#include "colour_interop.hpp"
#include "editable_string.hpp"
#include "auto_handlers.hpp"
#include <libncclient/c_shared_data.h>
#include "format_cache.hpp"

struct chat_thread;
struct chat_window;
struct font_render_context;

struct scrollbar_hack
{
    ///lines above bottom
    float scrolled = 0;
    float scrolled_this_frame = 0.f;

    float output_scroll_frac = 0.f;

    bool scrolling = false;
    bool should_update_scroll = false;

    void do_hack(int approx_num, bool set_scrollbar);
};

///has a frame, aka is a window
struct frameable
{
    ///how high the render area is in character sized rows;
    int render_height = 0;
};

struct realtime_script_run : frameable
{
    sf::Clock last_message;

    interop_vec_t parsed_data;
    bool focused = false;
    bool hovered = false;
    scrollbar_hack scroll_hack;
    bool open = true;
    bool was_open = true;

    std::string script_name;
    vec2f dim = {300, 300};
    bool set_size = true;

    vec2f current_tl_cursor_pos = {0,0};
    vec2f current_pos = {0,0};
    vec2i current_dim = {0,0};
    bool should_send_new_size = false;
    sf::Clock last_resize;

    format_cache cache;
};

struct terminal_imgui : serialisable, cacheable, frameable
{
    scrollbar_hack scroll_hack;

    std::map<std::string, chat_thread> chat_threads;

    std::map<int, realtime_script_run> realtime_script_windows;

    std::string current_user;
    bool one_time_user_insertion = true;

    bool consider_resetting_scrollbar = false;
    bool locked_to_bottom = false;
    bool lock_next_frame = true;
    bool focused = true;
    bool hovered = false;
    bool reset_scrollbar = true;
    editable_string command;

    auto_handler auto_handle;

    virtual void do_serialise(serialise& s, bool ser);

    void check_insert_user_command();

    void clear_terminal();
    void clear_chat();
    void clear_text();

    terminal_imgui();
    void render(font_render_context& font_select, sf::RenderWindow& win, bool refocus);
    void render_realtime_windows(font_render_context& font_select, c_shared_data data, int& was_closed_id);
    void bump_command_to_history();

    void add_text_from_server(c_shared_data shared, const std::string& in, chat_window& chat_win, bool server_command = true);

    void add_text_to_current_chat_thread(chat_window& chat_win, const std::string& text);

    ///returns -1 on none
    int get_id_of_focused_realtime_window();

    void invalidate();
    void last_line_invalidate();
};

struct chat_thread : serialisable, cacheable
{
    bool dirty = false;

    virtual void do_serialise(serialise& s, bool ser);
};

struct new_button
{
    std::string txt;
};

struct chat_window : serialisable, frameable
{
    scrollbar_hack scroll_hack;

    vec2f render_start = {0,0};
    vec2i dim = {500, 300};
    vec3f frame_col = {0.46f, 0.8f, 1.f};

    vec2f side_dim = {100, dim.y()};

    std::vector<std::string> side_buttons
    {
        "0000",
        "7001",
        "memes"
    };

    std::string selected = "0000";
    editable_string command;

    auto_handler auto_handle;

    virtual void do_serialise(serialise& s, bool ser);

    sf::Color get_frame_col();
    sf::Color get_highlight_col();

    bool focused = false;
    bool hovered = false;
    float border_size = 2.f;
    bool show_chat_in_main_window = true;

    void tick();

    void render(font_render_context& font_select, sf::RenderWindow& win, std::map<std::string, chat_thread>& threads, bool refocus);

    void set_side_channels(const std::vector<std::string>& sides);

    void invalidate();
};

#endif // IMGUI_UI_COMPONENTS_HPP_INCLUDED
