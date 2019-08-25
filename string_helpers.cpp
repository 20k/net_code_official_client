#include "string_helpers.hpp"
#include "auto_handlers.hpp"
#include "copy_handler.hpp"

#include <windows.h>

interop_vec_t string_to_interop(const std::string& str, bool render_specials, auto_handler& auto_handle, bool parse_for_autocompletes)
{
    interop_vec_t chars = build_from_colour_string(str, render_specials);

    if(str.size() < 2000)
        auto_handle.auto_colour(chars, false, parse_for_autocompletes);

    return chars;
}

interop_vec_t string_to_interop_no_autos(const std::string& str, bool render_specials)
{
    return build_from_colour_string(str, render_specials);
}

void de_newline(std::vector<interop_vec_t>& vec)
{
    return;

    auto in = vec;

    std::vector<interop_vec_t> rep;

    for(std::vector<interop_char>& vecs : in)
    {
        interop_vec_t cur;

        for(interop_char& chr : vecs)
        {
            if(chr.c == '\n')
            {
                if(cur.size() == 0)
                {
                    interop_char chr;
                    chr.c = ' ';
                    cur.push_back(chr);
                }

                rep.push_back(cur);
                cur = decltype(cur)();
                continue;
            }
            else
            {
                cur.push_back(chr);
            }
        }

        if(cur.size() == 0)
        {
            interop_char chr;
            chr.c = ' ';

            cur.push_back(chr);
        }

        rep.push_back(cur);
    }

    for(auto& i : rep)
    {
        if(i.size() == 0)
        {
            interop_char chr;
            chr.c = ' ';

            i.push_back(chr);
        }
    }

    if(vec.size() == 0)
    {
        interop_char tmp;
        tmp.c = ' ';
        tmp.col = {255, 255, 255};

        rep.push_back({tmp});
    }

    vec = rep;
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

std::string escape_str(const std::string& in)
{
    std::string str = in;

    for(int i=0; i < (int)str.size(); i++)
    {
        if(str[i] == '\"')
        {
            str.insert(str.begin() + i, '\\');
            i++;
            continue;
        }
    }

    return str;
}
