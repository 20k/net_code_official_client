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

    void build(float clip_width);

    std::string str;

    std::vector<screen_line> lines;
    std::vector<render_string> basic_render_strings;
};

struct driven_scrollbar
{
    vec2f window_size;
    float width = 14;
    float fraction = 1;
    float content_height = 0;

    void render(int trailing_blank_lines);
    void adjust_by_px(float py);
    void adjust_by_lines(float lines, int trailing_blank_lines);
};

struct text_manager
{
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
    void add_command_to_main_text(auto_handler& auto_handle, connection_send_data& send);

    virtual void default_controls(auto_handler& auto_handle, connection_send_data& send);
    virtual bool create_window(vec2f content_size, vec2f window_size);
    virtual void on_enter_text(std::string_view text, connection_send_data& send){}

    void render();

    void clear_text();
    void clear_command();

    std::vector<paragraph_string> paragraphs;
};

struct main_terminal2 : text_manager
{
    main_terminal2();

    bool title_dragging = false;
    vec2f title_start_pos;

    bool resize_dragging = false;
    vec2f resize_start_pos;

    std::string current_user;
};

struct child_terminal : text_manager
{
    child_terminal();
};

struct chat_thread2 : text_manager
{
    bool has_unread_message = false;

    bool was_focused = false;
    bool was_hovered = false;
    bool was_rendered = false;

    virtual bool create_window(vec2f content_size, vec2f window_size) override;
    virtual void on_enter_text(std::string_view text, connection_send_data& send) override;

    std::string name;

    std::vector<std::string> pending_input;
};

struct chat_manager
{
    bool show_chat_in_main_window = true;

    std::map<std::string, chat_thread2> chat_threads;

    std::vector<std::string> open_chat_channels;

    void extract_server_commands(nlohmann::json& in);
    void set_open_chat_channels(const std::vector<std::string>& channels);
    void add_text(const std::string& channel, const std::vector<std::string>& text);

    void default_controls(auto_handler& auto_handle, connection_send_data& send);
    void render();
};

void test_render_strings();

#endif // RENDER_STRING_HPP_INCLUDED
