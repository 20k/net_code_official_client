#ifndef IMGUI_UI_COMPONENTS_HPP_INCLUDED
#define IMGUI_UI_COMPONENTS_HPP_INCLUDED

#include <networking/serialisable.hpp>
#include "colour_interop.hpp"
#include "editable_string.hpp"
#include "auto_handlers.hpp"
#include <libncclient/c_shared_data.h>
#include "format_cache.hpp"
#include <SFML/System.hpp>

#define MAX_TEXT_HISTORY 200

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

    format_cache_2 cache;
};

struct chat_thread : serialisable, cacheable, free_function
{
    bool dirty = false;
    bool was_focused = false;
    bool was_hovered = false;
    editable_string command;
    std::string name;
};

struct chat_window : serialisable, frameable, free_function
{
    std::map<std::string, chat_thread> chat_threads;

    scrollbar_hack scroll_hack;

    vec2f render_start = {0,0};
    vec2i dim = {500, 300};
    vec3f frame_col = {0.46f, 0.8f, 1.f};

    vec2f side_dim = {100, dim.y()};

    std::vector<std::string> unprocessed_input;
    std::vector<std::string> side_buttons;

    auto_handler auto_handle;

    bool focused = false;
    bool hovered = false;
    float border_size = 2.f;
    bool show_chat_in_main_window = true;

    void tick();

    void render(bool refocus);

    void set_side_channels(const std::vector<std::string>& sides);

    void invalidate();
    void clear_chat();

    std::optional<editable_string*> get_focused_editable();
    std::optional<chat_thread*> get_focused_chat_thread();

    void add_text_to_focused(const std::string& str);
};

struct connection;

struct terminal_imgui : serialisable, cacheable, frameable, free_function
{
    scrollbar_hack scroll_hack;

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

    void check_insert_user_command();

    void clear_terminal();

    terminal_imgui();
    void render(vec2f window_size, bool refocus);
    void render_realtime_windows(connection& conn, int& was_closed_id);
    void bump_command_to_history();

    void add_text(const std::string& str);
    void add_text_from_server(std::string& current_user, const nlohmann::json& in, chat_window& chat_win, bool server_command = true);

    void add_text_to_current_chat_thread(chat_window& chat_win, const std::string& text);

    ///returns -1 on none
    int get_id_of_focused_realtime_window();
};

void clear_everything(terminal_imgui& term, chat_window& chat);
void invalidate_everything(terminal_imgui& term, chat_window& chat);
void last_line_invalidate_everything(terminal_imgui& term, chat_window& chat); ///cheap

#endif // IMGUI_UI_COMPONENTS_HPP_INCLUDED
