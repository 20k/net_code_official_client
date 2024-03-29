#ifndef RENDER_STRING_HPP_INCLUDED
#define RENDER_STRING_HPP_INCLUDED

#include <string_view>
#include <vec/vec.hpp>
#include <vector>
#include <string>
#include "auto_handlers.hpp"
#include "editable_string.hpp"
#include <networking/networking.hpp>
#include <nlohmann/json.hpp>
#include <networking/serialisable_fwd.hpp>
#include "ui_stack.hpp"

struct ImFont;
struct context;

///so. Wants to be a single paragraph of text, prebroken up into render units
///wants to be split up into screen sized lines, each of a known length, so its easy to reformat if the screen resizes
///hierarchy: render string is a paragraph
///made up of lines, which are screen sized
///each line is made up of render units, which are coloured

///no ok so:
///render strings should be built unconditionally, and then split up further by lines
struct render_string : serialisable, free_function
{
    vec3f colour;
    ///using a string_view would be too complicated, as it would require updating the views
    int start = 0;
    int length = 0;
};

struct screen_line
{
    //screen_line(std::string_view str);

    //vec2f dim;
    std::vector<render_string> strings;
};

struct paragraph_string : serialisable, free_function
{
    int unformatted_char_width = 0;
    vec2f dim;

    paragraph_string();
    paragraph_string(std::string in, bool include_specials, bool colour_like_terminal);

    void build(ImFont* font, float clip_width);
    void merge(const paragraph_string& in);

    std::string str;

    std::vector<screen_line> lines;
    std::vector<render_string> basic_render_strings;
};

struct driven_scrollbar
{
    bool locked_to_bottom = true;
    bool bottom_oriented = true;
    float pending_scroll = 0;

    void set_next_scroll(const std::string& window_title);
    void tick();
};

struct text_manager : serialisable, free_function
{
    vec2f window_tl;
    bool use_type_prompt = true;

    ImFont* font = nullptr;

    bool colour_like_terminal = false;
    bool new_terminal = true;
    bool open = true;
    bool unseen_text = false;
    int friendly_id = 0;
    float default_width = 400;
    bool has_set_viewport = false;

    ///derived properties
    bool was_visible = false;
    bool was_focused = false;
    int dock_id = -1;
    int last_trailing_blank_lines = 0;
    float window_title_offset = 0;

    std::string command_visual_prefix;

    ///todo: default keyboard controls for this, migrate it away from main()
    editable_string command;

    driven_scrollbar scrollbar;

    vec2f window_size;
    vec2f cached_character_size;

    void relayout(vec2f new_window_size);

    void add_main_text(std::string view, auto_handler& auto_handle);
    void add_main_text(std::string view);
    void add_command_to_main_text(auto_handler& auto_handle);

    void default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send);

    virtual std::string get_window_name();
    virtual bool create_window(context& ctx, vec2f content_size, vec2f window_size);
    virtual void destroy_window();
    virtual void on_pre_render(context& ctx, auto_handler& auto_handle, connection_send_data& send){}
    virtual void on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send);

    void render(context& ctx, auto_handler& auto_handle, connection_send_data& send);

    void limit_history();
    void clear_text();
    void clear_command();

    std::vector<paragraph_string> paragraphs;
};

struct terminal2 : text_manager
{
    int tag = 0;

    virtual void on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send) override;
    void extract_server_commands(context& ctx, nlohmann::json& in, auto_handler& auto_handle);
};

struct main_terminal2 : terminal2
{
    main_terminal2();

    virtual std::string get_window_name() override;
    virtual bool create_window(context& ctx, vec2f content_size, vec2f window_size) override;
    virtual void destroy_window() override;

    bool title_dragging = false;
    vec2f title_start_pos;

    bool resize_dragging = false;
    vec2f resize_start_pos;
};

struct child_terminal : terminal2
{
    child_terminal();

    virtual std::string get_window_name() override;
    virtual bool create_window(context& ctx, vec2f content_size, vec2f window_size) override;
};

struct terminal_manager2 : serialisable, free_function
{
    main_terminal2 primary;
    std::vector<child_terminal> secondary;

    void extract_server_commands(context& ctx, nlohmann::json& in, auto_handler& auto_handle);
    void render(context& ctx, auto_handler& auto_handle, connection_send_data& send);
    void default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send);
    void create_new_terminal();
};

struct chat_thread2 : text_manager
{
    virtual std::string get_window_name() override;
    virtual bool create_window(context& ctx, vec2f content_size, vec2f window_size) override;
    virtual void on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send) override;

    std::string friendly_name;
};

struct chat_manager  : serialisable, free_function
{
    bool windows_constrained = false;

    std::map<std::string, chat_thread2> chat_threads;

    std::vector<std::string> open_chat_channels;

    void extract_server_commands(nlohmann::json& in);
    void set_open_chat_channels(const std::vector<std::string>& channels);
    void add_text(const std::string& channel, const std::vector<std::string>& text);

    void default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send);
    void render(context& ctx, auto_handler& auto_handle, connection_send_data& send);
};

struct font_selector;

struct realtime_script_run2 : text_manager
{
    realtime_script_run2();

    steady_timer mouse_send_ratelimit;
    vec2f unprocessed_scrollwheel;

    int server_id = 0;
    std::string script_name;

    vec2f dim = {300, 300};

    bool is_focused = false;
    bool is_hovered = false;
    bool open = true;
    bool was_open = true;
    bool is_square_font = false;
    bool was_square_font = false;

    bool set_size = true;

    ui_stack stk;

    vec2f current_dim;
    bool should_send_new_size = false;
    steady_timer last_resize;

    uint64_t current_sequence_id = 0;
    uint64_t acked_sequence_id = 0;

    virtual std::string get_window_name() override;
    virtual bool create_window(context& ctx, vec2f content_size, vec2f in_window_size) override;
    virtual void destroy_window() override;
    void on_pre_render(context& ctx, auto_handler& auto_handle, connection_send_data& send);
    void default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send);
    void terminate(connection_send_data& send);
    //void render(connection_send_data& send);
};

struct realtime_script_manager2
{
    std::map<int, realtime_script_run2> windows;

    void extract_server_commands(font_selector& fonts, nlohmann::json& in);
    void default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send);
    //void render(connection_send_data& send);

    void render(context& ctx, auto_handler& auto_handle, connection_send_data& send);
};

void test_render_strings();

#endif // RENDER_STRING_HPP_INCLUDED
