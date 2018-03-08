#include "string_helpers.hpp"

#include <windows.h>

void render(sf::RenderWindow& win, const std::string& command, const std::vector<std::string>& text_history,
            const std::vector<int>& render_specials, int cursor_pos_idx, vec2f start, vec2f wrap_dim, float zero_bound,
            auto_handler& auto_handle)
{
    vec2f spos = start;

    std::vector<std::vector<interop_char>> all_interop;

    for(int i=0; i < (int)text_history.size(); i++)
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

    for(auto& i : formatted)
    {
        render_formatted_str(win, i, zero_bound);
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
