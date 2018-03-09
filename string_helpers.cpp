#include "string_helpers.hpp"
#include "auto_handlers.hpp"
#include "copy_handler.hpp"

#include <windows.h>

interop_vec_t string_to_interop(const std::string& str, bool render_specials, auto_handler& auto_handle)
{
    interop_vec_t chars = build_from_colour_string(str, render_specials);

    while(chars.size() > 0 && chars.back().c == '\n')
    {
        chars.pop_back();
    }

    auto_handle.auto_colour(chars);

    //chars.push_back({'\n'});

    return chars;
}

struct text_cache
{
    std::string str;
    vec3f col;

    friend bool operator<(const text_cache& l, const text_cache& r)
    {
        return std::tie(l.str, l.col.v[0], l.col.v[1], l.col.v[2])
             < std::tie(r.str, r.col.v[0], r.col.v[1], r.col.v[2]); // keep the same order
    }
};

void render_formatted_str(sf::RenderWindow& win, std::vector<formatted_char>& chars, float zero_bound)
{
    copy_handler* global_copy = get_global_copy_handler();

    auto [tl, br] = points_to_rect(global_copy->copy_start, global_copy->copy_end);

    /*static sf::RectangleShape shape;
    shape.setFillColor(sf::Color(255, 255, 255, 128));
    shape.setSize({char_inf::cwidth, char_inf::cheight});

    static sf::CircleShape circle;
    circle.setFillColor(hcol);

    circle.setRadius(2.f);
    circle.setOrigin(2.f, 2.f);*/

    sf::Color hcol = sf::Color(100, 100, 255, 255);

    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(char_inf::font_size);

    //std::map<text_cache, sf::Text> text_map;

    /*txt.setPosition(150, 150);
    txt.setFillColor(hcol);
    txt.setString(std::string(1, 'h'));*/

    for(formatted_char& c : chars)
    {
        vec2f pos = c.render_pos;

        if(pos.y() < zero_bound)
            continue;

        vec2f found_pos = round(pos);

        vec2f p1 = pos;
        vec2f p2 = pos + (vec2f){char_inf::cwidth, char_inf::cheight};

        vec3f col = c.ioc.col;

        /*text_cache cur;
        cur.str = std::string(1, c.ioc.c);
        cur.col = col;

        auto it = text_map.find(cur);

        if(it == text_map.end())
        {
            sf::Text next = txt;
            next.setFillColor(sf::Color(col.x(), col.y(), col.z()));
            next.setString(cur.str);

            text_map[cur] = next;
        }

        //sf::Text next = text_map[cur];

        auto fit = text_map.find(cur);

        fit->second.setPosition(found_pos.x(), found_pos.y());*/

        txt.setString(std::string(1, c.ioc.c));

        if(global_copy->held && rect_intersect(p1, p2, tl, br))
        {
            if(c.ioc.c != ' ')
            {
                txt.setOutlineThickness(0);
                txt.setOutlineColor(sf::Color(60, 60, 255, 255));

                txt.setFillColor(hcol);
            }
            else
            {
                txt.setString(std::string(1, '-'));

                txt.setOutlineThickness(0);
                txt.setOutlineColor(sf::Color(60, 60, 255, 255));

                txt.setFillColor(hcol);

                //vec2f hpos = found_pos + (vec2f){char_inf::cwidth, char_inf::cheight}/2.f;
                //circle.setPosition(hpos.x(), hpos.y());
                //win.draw(circle);
            }
        }
        else
        {
            txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));
            txt.setOutlineThickness(0);
        }

        txt.setPosition(found_pos.x(), found_pos.y());

        win.draw(txt);
        //win.draw(fit->second);
    }
}

void render(sf::RenderWindow& win, const std::string& command, const std::vector<interop_vec_t>& text_history,
            int cursor_pos_idx, vec2f start, vec2f wrap_dim, float zero_bound,
            auto_handler& auto_handle)
{
    vec2f spos = start;

    std::vector<std::vector<interop_char>> all_interop;

    int vertical_columns = ceil((float)win.getSize().y / char_inf::cheight);

    int min_start = (int)text_history.size() - vertical_columns;

    if(min_start < 0)
        min_start = 0;

    /*for(int i=min_start; i < (int)text_history.size(); i++)
    {
        const std::string& str = text_history[i];

        std::vector<interop_char> interop = string_to_interop(str, render_specials[i]);

        auto_handle.auto_colour(interop);

        all_interop.push_back(std::move(interop));
    }*/

    all_interop = text_history;

    std::string render_command = command;
    bool specials = true;

    if(render_command == "")
    {
        render_command = "`bType something here...`";
        specials = false;
    }

    auto icommand = string_to_interop(render_command, specials, auto_handle);

    interop_char curs;
    curs.col = {255, 255, 255};
    curs.c = '|';
    curs.is_cursor = true;

    if(cursor_pos_idx >= (int)icommand.size())
        icommand.push_back(curs);
    else if(cursor_pos_idx >= 0 && cursor_pos_idx < (int)icommand.size())
        icommand.insert(icommand.begin() + cursor_pos_idx, curs);

    all_interop.push_back(icommand);

    std::vector<std::vector<formatted_char>> formatted;

    for(auto& i : all_interop)
    {
        formatted.push_back(format_characters(i, spos, start, wrap_dim, zero_bound));
    }

    internally_format(formatted, start);

    get_global_copy_handler()->process_formatted(formatted);

    /*for(auto& i : formatted)
    {
        render_formatted_str(win, i, zero_bound);
    }*/

    for(int i=min_start; i < (int)formatted.size(); i++)
    {
        render_formatted_str(win, formatted[i], zero_bound);
    }
}

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

void set_clipboard_contents(const std::string& data)
{
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.size() + 1);
    memcpy(GlobalLock(hMem), data.c_str(), data.size());
    GlobalUnlock(hMem);

    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}
