#ifndef RENDER_STRING_HPP_INCLUDED
#define RENDER_STRING_HPP_INCLUDED

#include <string_view>
#include <vec/vec.hpp>
#include <vector>
#include <string>

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
    screen_line(std::string_view str);

    vec2f dim;
    std::vector<render_string> strings;
};

struct paragraph_string
{
    vec2f dim;

    paragraph_string();
    paragraph_string(std::string_view str, bool include_specials);

    void build(vec2f clipping_width);

    std::string str;

    std::vector<screen_line> lines;
    std::vector<render_string> basic_render_strings;
};

void test_render_strings();

#endif // RENDER_STRING_HPP_INCLUDED
