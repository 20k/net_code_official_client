#ifndef STRING_HELPERS_HPP_INCLUDED
#define STRING_HELPERS_HPP_INCLUDED

#include "colour_interop.hpp"

namespace char_inf
{
    inline int cwidth = 8;

    inline int cheight = 16;
    inline int cwbuf = 4;

    inline int font_size = 12;

    inline float extra_glyph_spacing = 0;
}

struct formatted_char
{
    interop_char ioc;
    vec2f internal_pos;
    vec2f render_pos;
    vec4f background_col = {0,0,0,0};
    bool copyable = true;
};

struct auto_handler;

interop_vec_t string_to_interop(const std::string& str, bool render_specials, auto_handler& auto_handle, bool parse_for_autocompletes = true);
interop_vec_t string_to_interop_no_autos(const std::string& str, bool render_specials);
void de_newline(std::vector<interop_vec_t>& vec);

inline
void get_height(const std::vector<interop_char>& interop, vec2f cpos, vec2f start, vec2f wrap_dim, int& lines, int last_lines)
{
    lines = 1;

    vec2f pos = cpos;
    pos.x() = start.x() + char_inf::cwbuf;
    pos.y() += char_inf::cheight;

    for(const auto& i : interop)
    {
        if((pos.x() >= wrap_dim.x() - char_inf::cwbuf || i.c == '\n') && !i.is_cursor)
        {
            pos.y() += char_inf::cheight;
            pos.x() = start.x() + char_inf::cwbuf;
            lines++;
        }

        if(i.c == '\n')
            continue;

        if(!i.is_cursor)
            pos.x() += char_inf::cwidth;
    }

    //pos.y() -= (last_lines) * char_inf::cheight + lines * char_inf::cheight;
}

///so new plan
///we want to format forward, but then afterwards
///move all the characters up by the height of the entire block
inline
std::vector<formatted_char> format_characters(const std::vector<interop_char>& interop, vec2f cpos, vec2f start, vec2f wrap_dim, int lines, int last_lines)
{
    std::vector<formatted_char> ret;

    vec2f pos = cpos;
    pos.x() = start.x() + char_inf::cwbuf;
    pos.y() += char_inf::cheight;

    for(int i=0; i < (int)interop.size(); i++)
    {
        const interop_char& ioc = interop[i];

        if((pos.x() >= wrap_dim.x() - char_inf::cwbuf || ioc.c == '\n') && !ioc.is_cursor)
        {
            pos.y() += char_inf::cheight;
            pos.x() = start.x() + char_inf::cwbuf;
        }

        if(ioc.c == '\n')
            continue;

        formatted_char formatted;
        formatted.ioc = ioc;

        if(ioc.is_cursor)
        {
            formatted.copyable = false;
        }

        if(ioc.is_cursor)
        {
            formatted.internal_pos = pos - (vec2f){char_inf::cwidth/2.f, 0.f};
        }
        else
        {
            formatted.internal_pos = pos;
        }

        if(!ioc.is_cursor)
            pos.x() += char_inf::cwidth;

        ret.push_back(formatted);
    }

    if(interop.size() == 0)
    {
        formatted_char formatted;
        formatted.ioc.c = ' ';
        formatted.ioc.col = {255, 255, 255};

        formatted.internal_pos = pos;
        formatted.copyable = true;

        ret.push_back(formatted);
    }

    /*for(formatted_char& i : ret)
    {
        i.internal_pos.y() -= (last_lines) * char_inf::cheight + lines * char_inf::cheight;
    }*/

    //pos.y() -= (last_lines) * char_inf::cheight + lines * char_inf::cheight;

    return ret;
}

///on the y axis
inline
float get_greatest_y(std::vector<formatted_char>& chars)
{
    float greatest_y = 0;

    for(formatted_char& i : chars)
    {
        if(i.internal_pos.y() > greatest_y)
        {
            greatest_y = i.internal_pos.y();
        }
    }

    return greatest_y;
}

inline
void internally_format(std::vector<std::vector<formatted_char>>& chars, vec2f start, float scroll_offset, float y_end)
{
    for(auto& k : chars)
    {
        for(formatted_char& i : k)
        {
            i.render_pos = i.internal_pos + (vec2f){0, -y_end + start.y() - char_inf::cheight * 1.5f + scroll_offset};

            i.render_pos = round(i.render_pos);
        }
    }
}

void render_formatted_str(sf::RenderWindow& win, std::vector<formatted_char>& chars, float zero_bound);

struct auto_handler;

std::string get_clipboard_contents();
void set_clipboard_contents(const std::string& str);

std::string escape_str(const std::string& in);

#endif // STRING_HELPERS_HPP_INCLUDED
