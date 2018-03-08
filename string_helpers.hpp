#ifndef STRING_HELPERS_HPP_INCLUDED
#define STRING_HELPERS_HPP_INCLUDED

static inline sf::Font font;

namespace char_inf
{
    int cwidth = 8;

    int cheight = 16;
    int cwbuf = 4;

    int font_size = 12;
}

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

struct formatted_char
{
    interop_char ioc;
    vec2f internal_pos;
    vec2f render_pos;
};

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

std::vector<formatted_char> format_characters(const std::vector<interop_char>& interop, vec2f& cpos, vec2f start, vec2f wrap_dim, float up_cutoff)
{
    std::vector<formatted_char> ret;

    vec2f& pos = cpos;
    pos.x() = start.x() + char_inf::cwbuf;
    pos.y() += char_inf::cheight;

    for(int i=0; i < interop.size(); i++)
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

void render_formatted_str(sf::RenderWindow& win, std::vector<formatted_char>& chars, float zero_bound)
{
    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(char_inf::font_size);

    for(formatted_char& c : chars)
    {
        vec2f pos = c.render_pos;

        //std::cout << pos << std::endl;

        vec2f found_pos = round(pos);

        if(found_pos.y() < zero_bound)
            continue;

        txt.setString(std::string(1, c.ioc.c));
        txt.setPosition(found_pos.x(), found_pos.y());

        vec3f col = c.ioc.col;

        txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));

        //vec2i dim = {txt.getGlobalBounds().width, txt.getGlobalBounds().height};

        win.draw(txt);
    }
}

void render(sf::RenderWindow& win, const std::string& command, const std::vector<std::string>& text_history,
            const std::vector<int>& render_specials, int cursor_pos_idx, vec2f start, vec2f wrap_dim, float zero_bound)
{
    vec2f spos = start;

    std::vector<std::vector<interop_char>> all_interop;

    for(int i=0; i < text_history.size(); i++)
    {
        const std::string& str = text_history[i];

        std::vector<interop_char> interop = string_to_interop(str, render_specials[i]);

        all_interop.push_back(interop);
    }

    std::string render_command = command;
    bool specials = true;

    if(render_command == "")
    {
        render_command = "`bType something here...`";
        specials = false;
    }

    auto icommand = string_to_interop(render_command, specials);

    interop_char curs;
    curs.col = {255, 255, 255};
    curs.c = '|';
    curs.is_cursor = true;

    if(cursor_pos_idx < 0)
        icommand.insert(icommand.begin(), curs);
    else if(cursor_pos_idx >= icommand.size())
        icommand.push_back(curs);
    else
        icommand.insert(icommand.begin() + cursor_pos_idx, curs);


    all_interop.push_back(icommand);

    std::vector<std::vector<formatted_char>> formatted;

    for(auto& i : all_interop)
    {
        formatted.push_back(format_characters(i, spos, start, wrap_dim, zero_bound));
    }

    /*if(formatted.size() > 0)
    {
        std::vector<formatted_char>& command_str = formatted.back();

        interop_char curs;
        curs.col = {255, 255, 255};
        curs.c = '|';


    }*/

    internally_format(formatted, start);

    for(auto& i : formatted)
    {
        render_formatted_str(win, i, zero_bound);
    }
}

#if 0
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

    auto icommand = string_to_interop(render_command, specials);

    render_str(win, icommand, current_pos, start, wrap_dim, cursor_pos_idx, zero_bound);

    current_pos.y() -= char_inf::cheight;

    int len = text_history.size();

    for(int i=len-1; i >= 0; i--)
    {
        const std::string& str = text_history[i];

        if(current_pos.y() >= wrap_dim.y() || current_pos.y() < zero_bound)
        {
            current_pos.y() -= char_inf::cheight;
            continue;
        }

        auto istr = string_to_interop(str, render_specials[i]);

        render_str(win, istr, current_pos, start, wrap_dim, -1, zero_bound);

        current_pos.y() -= char_inf::cheight;
    }
}
#endif // 0

std::string get_clipboard_contents()
{
    if(!OpenClipboard(NULL))
    {
        return std::string();
    }

    HANDLE hData = GetClipboardData(CF_TEXT);

    if (hData == nullptr)
    {
        CloseClipboard();
        return std::string();
    }

    // Lock the handle to get the actual text pointer
    char * pszText = static_cast<char*>( GlobalLock(hData) );

    std::string ntext(pszText);

    GlobalUnlock( hData );

    CloseClipboard();

    return ntext;
}

#endif // STRING_HELPERS_HPP_INCLUDED
