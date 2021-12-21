#ifndef RENDER_STRING_HPP_INCLUDED
#define RENDER_STRING_HPP_INCLUDED

#include <string_view>
#include <vec/vec.hpp>
#include <vector>
#include <string>
#include "auto_handlers.hpp"
#include "editable_string.hpp"

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

    void render(auto_handler& auto_handle);

    std::vector<paragraph_string> paragraphs;
};

void test_render_strings();

#endif // RENDER_STRING_HPP_INCLUDED
