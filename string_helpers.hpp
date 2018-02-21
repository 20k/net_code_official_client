#ifndef STRING_HELPERS_HPP_INCLUDED
#define STRING_HELPERS_HPP_INCLUDED

static inline sf::Font font;

namespace char_inf
{
    int cwidth = 8;

    int cheight = 16;
    int cwbuf = 4;
}

int get_num_lines(sf::RenderWindow& win, const std::vector<interop_char>& str)
{
    int width = win.getSize().x;

    int startx = char_inf::cwbuf;
    int num_lines = 0;

    for(int i=0; i < (int)str.size(); i++)
    {
        startx += char_inf::cwidth;

        if(str[i].c == '\n')
        {
            num_lines++;
            startx = char_inf::cwbuf;
            continue;
        }

        if(startx >= width - char_inf::cwbuf)
        {
            num_lines++;
            startx = char_inf::cwbuf;
            continue;
        }
    }

    return num_lines;
}


void render_str(sf::RenderWindow& win, const std::string& str, vec2f& cpos, bool render_specials)
{
    sf::Text txt;
    txt.setFont(font);

    vec2f pos = cpos;

    std::vector<interop_char> chars = build_from_colour_string(str, render_specials);

    while(chars.size() > 0 && chars.back().c == '\n')
    {
        chars.pop_back();
    }

    int num_lines = get_num_lines(win, chars);

    chars.push_back({'\n'});

    pos.y() -= num_lines * char_inf::cheight;

    for(int i=0; i < (int)chars.size(); i++)
    {
        if(pos.x() >= ((int)win.getSize().x) - char_inf::cwbuf || chars[i].c == '\n')
        {
            pos.y() += char_inf::cheight;
            pos.x() = char_inf::cwbuf;
        }

        if(chars[i].c == '\n')
            continue;

        vec2f found_pos = round(pos);

        txt.setString(std::string(1, chars[i].c));
        txt.setCharacterSize(12);
        txt.setPosition(found_pos.x(), found_pos.y());

        vec3f col = chars[i].col;

        txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));

        //vec2i dim = {txt.getGlobalBounds().width, txt.getGlobalBounds().height};

        pos.x() += char_inf::cwidth;

        win.draw(txt);
    }

    cpos.y() -= num_lines * char_inf::cheight;
}


#endif // STRING_HELPERS_HPP_INCLUDED
