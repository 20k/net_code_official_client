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

interop_vec_t string_to_interop_no_autos(const std::string& str, bool render_specials)
{
    interop_vec_t chars = build_from_colour_string(str, render_specials);

    while(chars.size() > 0 && chars.back().c == '\n')
    {
        chars.pop_back();
    }

    return chars;
}

void render_formatted_str(sf::RenderWindow& win, std::vector<formatted_char>& chars, float zero_bound)
{
    copy_handler* global_copy = get_global_copy_handler();

    auto [tl, br] = points_to_rect(global_copy->copy_start, global_copy->copy_end);

    sf::Color hcol = sf::Color(100, 100, 255, 255);

    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(char_inf::font_size);

    for(formatted_char& c : chars)
    {
        vec2f pos = c.render_pos;

        if(pos.y() < zero_bound)
            continue;

        vec2f found_pos = round(pos);

        vec2f p1 = pos;
        vec2f p2 = pos + (vec2f){char_inf::cwidth, char_inf::cheight};

        vec3f col = c.ioc.col;

        txt.setString(std::string(1, c.ioc.c));

        if(global_copy->held && rect_intersect(p1, p2, tl, br) && (global_copy->copy_end - global_copy->copy_start).length() >= MIN_SELECT_DISTANCE)
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
            }
        }
        else
        {
            txt.setFillColor(sf::Color(col.x(), col.y(), col.z(), 255));
            txt.setOutlineThickness(0);
        }

        txt.setPosition(found_pos.x(), found_pos.y());

        win.draw(txt);
    }
}

void render(sf::RenderWindow& win, std::string& command, const std::vector<interop_vec_t>& text_history,
            int& cursor_pos_idx, vec2f start, vec2f wrap_dim, float zero_bound,
            auto_handler& auto_handle, bool focused)
{
    vec2f spos = start;

    std::vector<std::vector<interop_char>> all_interop;

    int vertical_columns = ceil((float)win.getSize().y / char_inf::cheight);

    int min_start = (int)text_history.size() - vertical_columns;

    if(min_start < 0)
        min_start = 0;

    all_interop = text_history;

    std::string render_command = command;
    bool specials = true;

    if(render_command == "")
    {
        render_command = "`bType something here...`";
        specials = false;
    }

    auto icommand = string_to_interop(render_command, specials, auto_handle);

    auto_handle.handle_autocompletes(icommand, cursor_pos_idx, command);

    interop_char curs;
    curs.col = {255, 255, 255};
    curs.c = '|';
    curs.is_cursor = true;

    if(focused)
    {
        if(cursor_pos_idx >= (int)icommand.size())
            icommand.push_back(curs);
        else if(cursor_pos_idx >= 0 && cursor_pos_idx < (int)icommand.size())
            icommand.insert(icommand.begin() + cursor_pos_idx, curs);
    }

    all_interop.push_back(icommand);

    std::vector<std::vector<formatted_char>> formatted;

    for(auto& i : all_interop)
    {
        formatted.push_back(format_characters(i, spos, start, wrap_dim, zero_bound));
    }

    internally_format(formatted, start);

    if(focused)
        get_global_copy_handler()->process_formatted(formatted);

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
