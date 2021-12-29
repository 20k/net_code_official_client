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
#include "imgui_ui_components.hpp"

struct ImFont;
struct context;

///so. Wants to be a single paragraph of text, prebroken up into render units
///wants to be split up into screen sized lines, each of a known length, so its easy to reformat if the screen resizes
///hierarchy: render string is a paragraph
///made up of lines, which are screen sized
///each line is made up of render units, which are coloured

///no ok so:
///render strings should be built unconditionally, and then split up further by lines
struct render_string
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

struct paragraph_string
{
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

    vec2f window_size;
    float width = 14;
    float fraction = 1;
    float content_height = 0;

    void render(ImFont* font, int trailing_blank_lines);
    void adjust_by_px(float py);
    void adjust_by_lines(ImFont* font, float lines, int trailing_blank_lines);
};

struct text_manager
{
    vec2f window_tl;
    bool use_type_prompt = true;

    ImFont* font = nullptr;

    bool colour_like_terminal = false;
    bool new_terminal = true;
    bool open = true;
    bool unseen_text = false;
    int friendly_id = 0;

    ///derived properties
    bool was_visible = false;
    bool was_focused = false;
    int dock_id = -1;

    std::string command_visual_prefix;

    ///todo: default keyboard controls for this, migrate it away from main()
    editable_string command;

    driven_scrollbar scrollbar;

    vec2f window_size;
    vec2f cached_character_size;

    bool should_reset_scrollbar = true;
    bool scrollbar_at_bottom = false;

    void relayout(vec2f new_window_size);

    void add_main_text(std::string view, auto_handler& auto_handle);
    void add_main_text(std::string view);
    void add_command_to_main_text(auto_handler& auto_handle);

    void default_controls(context& ctx, auto_handler& auto_handle, connection_send_data& send);
    virtual bool create_window(context& ctx, vec2f content_size, vec2f window_size);
    virtual void destroy_window();
    virtual void on_pre_render(context& ctx, auto_handler& auto_handle, connection_send_data& send){}
    virtual void on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send);

    void render(context& ctx, auto_handler& auto_handle, connection_send_data& send);

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

    virtual bool create_window(context& ctx, vec2f content_size, vec2f window_size) override;
    virtual void destroy_window() override;

    vec2f window_pos;

    bool title_dragging = false;
    vec2f title_start_pos;

    bool resize_dragging = false;
    vec2f resize_start_pos;
};

struct child_terminal : terminal2
{
    child_terminal();
};

struct terminal_manager2
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
    bool has_unread_message = false;

    bool was_focused = false;
    bool was_hovered = false;
    bool was_rendered = false;

    virtual bool create_window(context& ctx, vec2f content_size, vec2f window_size) override;
    virtual void on_enter_text(context& ctx, std::string_view text, auto_handler& auto_handle, connection_send_data& send) override;

    std::string friendly_name;

    std::vector<std::string> pending_input;
};

struct chat_manager
{
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
