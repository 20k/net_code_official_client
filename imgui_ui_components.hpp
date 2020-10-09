#ifndef IMGUI_UI_COMPONENTS_HPP_INCLUDED
#define IMGUI_UI_COMPONENTS_HPP_INCLUDED

#include <networking/serialisable.hpp>
#include "colour_interop.hpp"
#include "editable_string.hpp"
#include "auto_handlers.hpp"
#include "format_cache.hpp"
#include <toolkit/clock.hpp>

#define MAX_TEXT_HISTORY 200
#include <imgui/imgui.h>

struct chat_thread;
struct chat_window;
struct auth_manager;

struct scrollbar_hack
{
    ///lines above bottom
    float scrolled = 0;
    float scrolled_this_frame = 0.f;

    bool scrolling = false;
    bool should_update_scroll = false;

    void do_hack(int approx_num, bool set_scrollbar, format_cache_2& cache, vec2f dim);
};

struct ui_element
{
    std::string type;
    std::string element_id;
    std::vector<nlohmann::json> arguments;

    bool was_hovered = false;
};

struct ui_stack
{
    std::vector<ui_element> elements;
    std::vector<std::string> typelist;
};

struct realtime_script_run
{
    steady_timer last_message;

    interop_vec_t parsed_data;
    bool focused = false;
    bool hovered = false;
    scrollbar_hack scroll_hack;
    bool open = true;
    bool was_open = true;
    bool is_square_font = false;
    bool was_square_font = false;

    ui_stack stk;

    std::string script_name;
    vec2f dim = {300, 300};
    bool set_size = true;

    vec2f current_tl_cursor_pos = {0,0};
    vec2f current_pos = {0,0};
    vec2i current_dim = {0,0};
    bool should_send_new_size = false;
    steady_timer last_resize;

    format_cache_2 cache;
};

struct connection;
struct font_selector;

struct realtime_script_manager
{
    std::map<int, realtime_script_run> windows;

    void render_realtime_windows(connection& conn, int& was_closed_id, font_selector& fonts, auto_handler& auto_handle, bool is_linear_colour);

    ///returns -1 on none
    int get_id_of_focused_realtime_window();
};

struct chat_thread : serialisable, cacheable, free_function
{
    bool dirty = false;
    bool was_focused = false;
    bool was_hovered = false;
    bool was_rendered = false;
    editable_string command;
    std::string name;
    scrollbar_hack scroll_hack;
};

struct chat_window : serialisable, free_function
{
    std::map<std::string, chat_thread> chat_threads;

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

    void render(bool refocus);

    void set_side_channels(const std::vector<std::string>& sides);

    bool any_cache_invalid();
    void clear_chat();

    std::optional<editable_string*> get_focused_editable();
    std::optional<chat_thread*> get_focused_chat_thread();

    void add_text_to_focused(const std::string& str);
};

struct render_window;
struct terminal_manager;;

struct terminal_imgui : serialisable, cacheable, free_function
{
    scrollbar_hack scroll_hack;

    //begin main terminal only properties
    bool title_dragging = false;
    ImVec2 title_start_pos = ImVec2(0,0);

    bool resize_dragging = false;
    ImVec2 resize_start_pos = ImVec2(0,0);
    //end main terminal only properties

    bool is_main_terminal = false;
    bool new_terminal = true;

    int friendly_id = 0;

    bool open = true;

    std::string current_user;
    bool one_time_user_insertion = true;

    bool consider_resetting_scrollbar = false;
    bool locked_to_bottom = false;
    bool lock_next_frame = true;
    bool focused = true;
    bool hovered = false;
    bool reset_scrollbar = true;
    editable_string command;

    void check_insert_user_command();

    void clear_terminal();

    void render(terminal_manager& terminals, render_window& win, vec2f window_size, bool refocus, int extra_id);
    void bump_command_to_history(auto_handler& auto_handle);

    void add_text(const std::string& str, auto_handler& auto_handle);
    void extend_text(const std::string& str, auto_handler& auto_handle);
    //void add_text_from_server(auth_manager& auth_manage, std::string& current_user, const nlohmann::json& in, chat_window& chat_win, font_selector& fonts);
};

struct terminal_manager : serialisable, free_function
{
    std::map<int, terminal_imgui> sub_terminals;
    terminal_imgui main_terminal;
    auto_handler auto_handle;

    size_t gid = 1;

    terminal_manager();
    void render(render_window& win, vec2f window_size, bool refocus);
    bool all_cache_valid();
    void invalidate_visual_cache();

    terminal_imgui* get_focused_terminal();
    int get_focused_terminal_id();

    void make_new_terminal();
};

void process_text_from_server(terminal_manager& terminals, auth_manager& auth_manage, std::string& current_user, nlohmann::json& data, chat_window& chat_win, font_selector& fonts, realtime_script_manager& realtime_scripts);

void clear_everything(terminal_manager& term, chat_window& chat);
void invalidate_everything(terminal_manager& term, chat_window& chat);
void last_line_invalidate_everything(terminal_manager& term, chat_window& chat); ///cheap

#endif // IMGUI_UI_COMPONENTS_HPP_INCLUDED
