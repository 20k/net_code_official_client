#ifndef COLOUR_INTEROP_HPP_INCLUDED
#define COLOUR_INTEROP_HPP_INCLUDED

#include <map>
#include <vec/vec.hpp>
#include <string>
#include <vector>
#include <networking/serialisable.hpp>

inline
std::array<vec3f, 256> get_cmap()
{
    std::array<vec3f, 256> colour_map = {};

    ///sigh
    ///need to sample with bloom off
    colour_map['A'] = {240, 240, 240};
    colour_map['B'] = {234, 241, 241};
    colour_map['C'] = {206, 202, 203};
    colour_map['D'] = {243, 8, 8};
    colour_map['E'] = {242, 164, 160};
    colour_map['F'] = {243, 166, 3};
    colour_map['G'] = {243, 231, 130};
    colour_map['H'] = {243, 241, 7};
    colour_map['I'] = {243, 243, 113};
    colour_map['J'] = {243, 243, 7};
    colour_map['K'] = {242, 242, 193};
    colour_map['L'] = {42, 243, 8};
    colour_map['M'] = {231, 243, 204};
    colour_map['N'] = {5, 243, 243};
    colour_map['O'] = {179, 243, 243};
    colour_map['P'] = {8, 143, 242};
    colour_map['Q'] = {213, 242, 243};
    colour_map['R'] = {4, 8, 243};
    colour_map['S'] = {149, 228, 242};
    colour_map['T'] = {237, 60, 242};
    colour_map['U'] = {241, 236, 241};
    colour_map['V'] = {242, 6, 242};
    colour_map['W'] = {241, 186, 241};
    colour_map['X'] = {241, 6, 142};
    colour_map['Y'] = {240, 123, 195};
    colour_map['Z'] = {11, 16, 40};

    colour_map['a'] = {1,1,1};
    colour_map['b'] = {60,60,60};
    colour_map['c'] = {118,118,118};
    colour_map['d'] = {158,1,1};
    colour_map['e'] = {185,49,49};
    colour_map['f'] = {211,79,2};
    colour_map['g'] = {138,88,50};
    colour_map['h'] = {216,163,2};
    colour_map['i'] = {232,185,71};
    colour_map['j'] = {186,188,1};
    colour_map['k'] = {73,90,34};
    colour_map['l'] = {38,183,1};
    colour_map['m'] = {33,56,26};
    colour_map['n'] = {1,87,100};
    colour_map['o'] = {46,75,79};
    colour_map['p'] = {2,135,213};
    colour_map['q'] = {51,97,127};
    colour_map['r'] = {2,2,122};
    colour_map['s'] = {82,143,207};
    colour_map['t'] = {106,27,158};
    colour_map['u'] = {68,46,81};
    colour_map['v'] = {176,1,123};
    colour_map['w'] = {192,52,160};
    colour_map['x'] = {173,1,37};
    colour_map['y'] = {140,42,72};
    colour_map['z'] = {15,17,20};

    colour_map['0'] = {255, 20, 255};
    colour_map['1'] = {255, 20, 20};
    colour_map['2'] = {255, 140, 40};
    colour_map['3'] = {255, 255, 40};
    colour_map['4'] = {60, 255, 60};

    ///used as a hack in autocompletes
    colour_map['|'] = colour_map['c'];

    return colour_map;
}

inline
std::optional<vec3f> letter_to_colour(char in)
{
    static auto cmap = get_cmap();

    vec3f col = cmap[in];

    if(col.x() == 0 && col.y() == 0 && col.z() == 0)
        return std::nullopt;

    return col;
}

struct interop_char : serialisable, free_function
{
    int c = 'A';
    vec3f col = {255,255,255};
    bool is_cursor = false;
    bool coloured = false;
};

using interop_vec_t = std::vector<interop_char>;

inline
void strip_interop(interop_vec_t& in)
{
    if(in.size() <= 2)
        return;

    if(in.front().c == '`' && in.back().c == '`' && isalnum(in[1].c) && in[1].c != '`')
    {
        in.pop_back();
        in.erase(in.begin());
        in.erase(in.begin());
    }
}

inline
interop_vec_t build_from_colour_string(const std::string& in, bool include_specials)
{
    interop_vec_t ret;

    if(in.size() == 0)
        return ret;

    ret.reserve(in.size());

    bool found_colour = false;
    bool set_colour = false;

    vec3f last_colour = letter_to_colour('A').value();

    bool term = false;

    interop_vec_t current_color_buf;

    int length = in.size();

    for(int i=0; i < length; i++)
    {
        char cur = in[i];

        char next = 0;
        char next_2 = 0;
        //char prev = 0;

        if(i < length - 1)
            next = in[i + 1];

        if(i < length - 2)
            next_2 = in[i + 2];

        //if(i > 0)
        //    prev = in[i - 1];

        term = false;

        // \\`
        if(cur == '\\' && next == '\\' && next_2 == '`')
        {
            if(include_specials)
            {
                interop_char c;
                c.c = '\\';

                if(found_colour)
                {
                    c.col = last_colour;
                    c.coloured = found_colour;
                }

                current_color_buf.push_back(c);
            }

            interop_char c;
            c.c = '\\';

            if(found_colour)
            {
                c.col = last_colour;
                c.coloured = found_colour;
            }

            current_color_buf.push_back(c);

            i++;
            continue;
        }

        // \`
        if(cur == '\\' && next == '`')
        {
            if(include_specials)
            {
                interop_char c;
                c.c = '\\';

                if(found_colour)
                {
                    c.col = last_colour;
                    c.coloured = found_colour;
                }

                current_color_buf.push_back(c);
            }

            interop_char c;
            c.c = '`';

            if(found_colour)
            {
                c.col = last_colour;
                c.coloured = found_colour;
            }

            current_color_buf.push_back(c);

            i++;
            continue;
        }

        // ` initial
        if(cur == '`' && !found_colour && !set_colour)
        {
            strip_interop(current_color_buf);
            ret.insert(ret.end(), current_color_buf.begin(), current_color_buf.end());
            current_color_buf.clear();

            set_colour = true;

            interop_char c;
            c.c = cur;

            if(auto val = letter_to_colour(next); val.has_value())
            {
                c.col = *val;
                c.coloured = true;
            }

            current_color_buf.push_back(c);

            continue;
        }

        // ``, the second `
        if(cur == '`' && set_colour)
        {
            set_colour = false;
            found_colour = true;

            /*interop_char c;
            c.c = '¬';
            c.col = cmap['A'];

            current_color_buf.push_back(c);*/
        }

        // `X, the X
        if(set_colour)
        {
            set_colour = false;
            found_colour = true;

            std::optional<vec3f> col = letter_to_colour(cur);

            last_colour = col.value_or(letter_to_colour('A').value());

            interop_char c;
            c.c = cur;

            if(col.has_value())
            {
                c.col = *col;
                c.coloured = true;
            }

            /*c.col = cmap[last_col];
            c.coloured = true;*/

            current_color_buf.push_back(c);

            continue;
        }

        //regular character
        if(cur != '`' && cur != '\n')
        {
            interop_char c;
            c.c = cur;
            c.col = last_colour;
            c.coloured = found_colour;

            current_color_buf.push_back(c);

            continue;
        }

        // `ASDF`, second tick, or \n
        if((cur == '`' && found_colour) || cur == '\n')
        {
            interop_char c;
            c.c = cur;
            c.col = last_colour;
            c.coloured = found_colour;

            current_color_buf.push_back(c);

            if(!include_specials)
                strip_interop(current_color_buf);

            ret.insert(ret.end(), current_color_buf.begin(), current_color_buf.end());
            current_color_buf.clear();

            found_colour = false;
            set_colour = false;
            last_colour = letter_to_colour('A').value();
            term = true;

            continue;
        }
    }

    if(!term)
    {
        ret.insert(ret.end(), current_color_buf.begin(), current_color_buf.end());
        current_color_buf.clear();
    }

    return ret;
}

#endif // COLOUR_INTEROP_HPP_INCLUDED
