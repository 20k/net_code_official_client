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
void get_height(const std::vector<interop_char>& interop, vec2f cpos, vec2f start, vec2f wrap_dim, int& lines, vec2f cdim)
{
    lines = 1;

    vec2f pos = cpos;
    pos.x() = start.x() + char_inf::cwbuf;
    pos.y() += cdim.y();

    for(const auto& i : interop)
    {
        if((pos.x() >= wrap_dim.x() - char_inf::cwbuf || i.c == '\n') && !i.is_cursor)
        {
            pos.y() += cdim.y();
            pos.x() = start.x() + char_inf::cwbuf;
            lines++;
        }

        if(i.c == '\n')
            continue;

        if(!i.is_cursor)
            pos.x() += cdim.x();
    }
}

///so new plan
///we want to format forward, but then afterwards
///move all the characters up by the height of the entire block
inline
std::vector<formatted_char> format_characters(const std::vector<interop_char>& interop, vec2f cpos, vec2f start, vec2f wrap_dim, int lines, int last_lines, vec2f cdim)
{
    std::vector<formatted_char> ret;

    vec2f pos = cpos;
    pos.x() = start.x() + char_inf::cwbuf;
    pos.y() += cdim.y();

    for(int i=0; i < (int)interop.size(); i++)
    {
        const interop_char& ioc = interop[i];

        if((pos.x() >= wrap_dim.x() - char_inf::cwbuf || ioc.c == '\n') && !ioc.is_cursor)
        {
            pos.y() += cdim.y();
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
            formatted.internal_pos = pos - (vec2f){cdim.x()/2.f, 0.f};
        }
        else
        {
            formatted.internal_pos = pos;
        }

        if(!ioc.is_cursor)
            pos.x() += cdim.x();

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

struct auto_handler;

std::string get_clipboard_contents();
void set_clipboard_contents(const std::string& str);

std::string escape_str(const std::string& in);

#endif // STRING_HELPERS_HPP_INCLUDED
