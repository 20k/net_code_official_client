#ifndef STRING_HELPERS_HPP_INCLUDED
#define STRING_HELPERS_HPP_INCLUDED

#include <SFML/Graphics.hpp>
#include "colour_interop.hpp"

inline sf::Font font;

namespace char_inf
{
    inline int cwidth = 8;

    inline int cheight = 16;
    inline int cwbuf = 4;

    inline int font_size = 12;
}

struct formatted_char
{
    interop_char ioc;
    vec2f internal_pos;
    vec2f render_pos;
    vec4f background_col = {0,0,0,0};
};

#if 0
int get_num_lines(vec2f start, vec2f dim, const interop_vec_t& str)
{
    int width = dim.x();

    int startx = start.x() + char_inf::cwbuf;
    int num_lines = 0;

    for(int i=0; i < (int)str.size(); i++)
    {
        if(startx >= width - char_inf::cwbuf || str[i].c == '\n')
        {
            num_lines++;
            startx = char_inf::cwbuf + start.x();
            continue;
        }

        startx += char_inf::cwidth;
    }

    return num_lines;
}

void render_individual(sf::RenderWindow& win, char c, vec2f pos, sf::Text& txt)
{
    vec2f cpos = pos;
    cpos.x() -= char_inf::cwidth/2.f;

    cpos = round(cpos);

    txt.setString(std::string(1, c));
    txt.setPosition(cpos.x(), cpos.y());
    txt.setFillColor(sf::Color::White);

    win.draw(txt);
}

void render_str(sf::RenderWindow& win, const interop_vec_t& chars, vec2f& cpos, vec2f start, vec2f wrap_dim, int render_cursor_at, float zero_bound)
{
    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(char_inf::font_size);

    vec2f pos = cpos;

    int num_lines = get_num_lines(start, wrap_dim, chars);

    pos.y() -= num_lines * char_inf::cheight;

    for(int i=0; i < (int)chars.size(); i++)
    {
        if(chars[i].c == '\n' && render_cursor_at == i)
        {
            render_individual(win, '|', pos, txt);
        }

        if(pos.x() >= wrap_dim.x() - char_inf::cwbuf || chars[i].c == '\n')
        {
            pos.y() += char_inf::cheight;
            pos.x() = start.x() + char_inf::cwbuf;
        }

        if(chars[i].c == '\n')
        {
            continue;
        }

        if(pos.y() <= zero_bound)
        {
            pos.x() += char_inf::cwidth;
            continue;
        }


        vec2f found_pos = round(pos);

        txt.setString(std::string(1, chars[i].c));
        //txt.setString(std::to_string(num_lines));
        txt.setPosition(found_pos.x(), found_pos.y());

        vec3f col = chars[i].col;

        txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));

        //vec2i dim = {txt.getGlobalBounds().width, txt.getGlobalBounds().height};

        win.draw(txt);

        if(render_cursor_at == i)
            render_individual(win, '|', pos, txt);

        pos.x() += char_inf::cwidth;
    }

    if(render_cursor_at >= (int)chars.size())
        render_individual(win, '|', pos, txt);

    cpos.y() -= num_lines * char_inf::cheight;
}
#endif // 0

inline
void render_str(sf::RenderWindow& win, const interop_vec_t& chars, vec2f& cpos, vec2f start, vec2f wrap_dim, float zero_bound)
{
    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(char_inf::font_size);

    vec2f pos = cpos;

    for(int i=0; i < (int)chars.size(); i++)
    {
        if(pos.x() >= wrap_dim.x() - char_inf::cwbuf || chars[i].c == '\n')
        {
            pos.y() += char_inf::cheight;
            pos.x() = start.x() + char_inf::cwbuf;
        }

        if(chars[i].c == '\n')
            continue;

        if(pos.y() <= zero_bound)
        {
            pos.x() += char_inf::cwidth;
            continue;
        }

        vec2f found_pos = round(pos);

        txt.setString(std::string(1, chars[i].c));
        txt.setPosition(found_pos.x(), found_pos.y());

        vec3f col = chars[i].col;

        txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));

        win.draw(txt);

        pos.x() += char_inf::cwidth;
    }
}

inline
interop_vec_t string_to_interop(const std::string& str, bool render_specials)
{
    interop_vec_t chars = build_from_colour_string(str, render_specials);

    while(chars.size() > 0 && chars.back().c == '\n')
    {
        chars.pop_back();
    }

    //chars.push_back({'\n'});

    return chars;
}

inline
std::vector<formatted_char> format_characters(const std::vector<interop_char>& interop, vec2f& cpos, vec2f start, vec2f wrap_dim, float up_cutoff)
{
    std::vector<formatted_char> ret;

    vec2f& pos = cpos;
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
void internally_format(std::vector<std::vector<formatted_char>>& chars, vec2f start)
{
    float greatest_y = 0;

    for(auto& i : chars)
    {
        greatest_y = std::max(get_greatest_y(i), greatest_y);
    }

    for(auto& k : chars)
    {
        for(formatted_char& i : k)
        {
            i.render_pos = i.internal_pos + (vec2f){0, -greatest_y + start.y() - char_inf::cheight};
        }
    }
}

void render_formatted_str(sf::RenderWindow& win, std::vector<formatted_char>& chars, float zero_bound);

struct auto_handler;

///this might look suspiciously like bad api design
///and you'd be right
void render(sf::RenderWindow& win, const std::string& command, const std::vector<std::string>& text_history,
       const std::vector<int>& render_specials, int cursor_pos_idx, vec2f start, vec2f wrap_dim, float zero_bound,
       auto_handler& auto_handle);

std::string get_clipboard_contents();
void set_clipboard_contents(const std::string& str);

#endif // STRING_HELPERS_HPP_INCLUDED
