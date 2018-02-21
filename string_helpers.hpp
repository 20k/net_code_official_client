#ifndef STRING_HELPERS_HPP_INCLUDED
#define STRING_HELPERS_HPP_INCLUDED

static inline sf::Font font;

namespace char_inf
{
    int cwidth = 8;

    int cheight = 16;
    int cwbuf = 4;
}

int get_num_lines(vec2f start, vec2f dim, const std::vector<interop_char>& str)
{
    int width = dim.x();

    int startx = start.x() + char_inf::cwbuf;
    int num_lines = 0;

    for(int i=0; i < (int)str.size(); i++)
    {
        startx += char_inf::cwidth;

        if(str[i].c == '\n')
        {
            num_lines++;
            startx = char_inf::cwbuf + start.x();
            continue;
        }

        if(startx >= width - char_inf::cwbuf)
        {
            num_lines++;
            startx = char_inf::cwbuf + start.x();
            continue;
        }
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

void render_str(sf::RenderWindow& win, const std::string& str, vec2f& cpos, bool render_specials, vec2f start, vec2f wrap_dim, int render_cursor_at, float zero_bound)
{
    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(12);

    vec2f pos = cpos;

    std::vector<interop_char> chars = build_from_colour_string(str, render_specials);

    while(chars.size() > 0 && chars.back().c == '\n')
    {
        chars.pop_back();
    }

    int num_lines = get_num_lines(start, wrap_dim, chars);

    chars.push_back({'\n'});

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
            //pos.x() = char_inf::cwbuf;
            pos.x() = start.x() + char_inf::cwbuf;
        }

        if(pos.y() <= zero_bound)
        {
            continue;
        }

        if(chars[i].c == '\n')
        {
            continue;
        }

        vec2f found_pos = round(pos);

        txt.setString(std::string(1, chars[i].c));
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

void render(sf::RenderWindow& win, const std::string& command, const std::vector<std::string>& text_history,
            const std::vector<int>& render_specials, int cursor_pos_idx, vec2f start, vec2f wrap_dim, float zero_bound)
{
    vec2f start_pos = {start.x() + char_inf::cwbuf, start.y() - char_inf::cheight};

    //vec2f start_pos = {char_inf::cwbuf + start_pos.x(), (win.getSize().y - char_inf::cheight) + start_pos.y()};
    vec2f current_pos = start_pos;

    std::string render_command = command;
    bool specials = true;

    if(render_command == "")
    {
        render_command = "`bType something here...`";
        specials = false;
    }

    render_str(win, render_command, current_pos, specials, start, wrap_dim, cursor_pos_idx, zero_bound);

    current_pos.y() -= char_inf::cheight;

    int len = text_history.size();

    for(int i=len-1; i >= 0; i--)
    {
        std::string str = text_history[i];

        if(current_pos.y() >= wrap_dim.y() || current_pos.y() < zero_bound)
            continue;

        render_str(win, str, current_pos, render_specials[i], start, wrap_dim, -1, zero_bound);

        current_pos.y() -= char_inf::cheight;
    }

    /*std::string cursor_icon = "|";

    vec2f to_render_curs = start_pos;
    to_render_curs.x() += char_inf::cwidth * cursor_pos_idx - char_inf::cwidth/2.f;

    render_str(win, cursor_icon, to_render_curs, false, start, wrap_dim);*/
}

#endif // STRING_HELPERS_HPP_INCLUDED
