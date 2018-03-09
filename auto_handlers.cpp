#include "auto_handlers.hpp"
#include "string_helpers.hpp"

bool strip_input(std::string& in)
{
    bool found = false;

    while(in.size() > 0 && in.back() == '?')
    {
        in.pop_back();
        found = true;
    }

    return found;
}

bool interop_starts_with(const std::vector<interop_char>& chs, int idx, const std::string& str)
{
    if(idx < 0 || idx + (int)str.size() > (int)chs.size())
        return false;

    int offset = 0;

    for(int i=idx; i < idx + (int)str.size(); i++)
    {
        if(str[offset] == '?' && isalpha(chs[i].c))
            return false;

        if(str[offset] != '?' && chs[i].c != str[offset])
            return false;

        offset++;
    }

    return true;
}

void force_col(std::vector<interop_char>& chs, int start, int fin, vec3f col)
{
    for(int i=start; i < fin && i < (int)chs.size(); i++)
    {
        chs[i].col = col;
        chs[i].coloured = true;
    }
}

void interop_colour_string(std::vector<interop_char>& chs, vec3f col)
{
    std::string starter;

    bool escaped = false;

    int start = 0;

    for(int i=0; i < (int)chs.size(); i++)
    {
        if(chs[i].coloured)
            continue;

        std::string ch = std::string(1, chs[i].c);

        if(!escaped)
        {
            ///DOUBLE QUOTE
            if(ch == "\"" && starter == "")
            {
                starter = ch;
                start = i;
                continue;
            }

            ///SINGLE QUOTE
            if(ch == "\'" && starter == "")
            {
                starter = ch;
                start = i;
                continue;
            }

            if(ch == "\\")
            {
                escaped = true;
                continue;
            }
        }

        escaped = false;

        if(ch == starter && (ch == "\"" || ch == "\'"))
        {
            force_col(chs, start, i+1, col);
            starter = "";
            continue;
        }
    }
}

void interop_colour_numbers(std::vector<interop_char>& chs, vec3f col)
{
    for(int i=0; i < (int)chs.size(); i++)
    {
        bool can_col = false;

        if(i == 0)
            can_col = true;
        else
            can_col = !isalnum(chs[i-1].c);

        if(isdigit(chs[i].c) && !chs[i].coloured && can_col)
        {
            int start = i;

            for(; i < (int)chs.size(); i++)
            {
                if(!isalnum(chs[i].c))
                {
                    force_col(chs, start, i, col);
                    break;
                }

                if(!isdigit(chs[i].c))
                    break;
            }
        }
    }
}

void interop_colour(std::vector<interop_char>& chs, int idx, const std::string& str, vec3f col)
{
    if(!interop_starts_with(chs, idx, str))
        return;

    std::string op = str;

    strip_input(op);

    for(int i=idx; i < idx + (int)op.size(); i++)
    {
        if(chs[i].coloured)
            continue;

        chs[i].col = col;
    }
}

void auto_handler::auto_colour(std::vector<interop_char>& ret, bool colour_special)
{
    std::map<std::string, vec3f> cols
    {
        {"#fs.", {60, 255, 60}},
        {"#hs.", {255, 255, 40}},
        {"#ms.", {255, 140, 40}},
        {"#ls.", {255, 20, 20}},
        {"#ns.", {255, 20, 255}},
    };

    vec3f pale_blue = {120, 120, 255};
    vec3f pale_red = {255, 60, 60};

    //if(colour_special)
    {
        cols["function?"] = pale_blue;
        cols["while?"] = pale_blue;
        cols["for?"] = pale_blue;
        cols["if?"] = pale_blue;
        cols["return?"] = pale_blue;
        cols["{"] = pale_red;
        cols["}"] = pale_red;
        cols["["] = pale_red;
        cols["]"] = pale_red;
        cols[";"] = pale_red;
    }

    for(auto& i : cols)
    {
        for(int kk=0; kk < ret.size(); kk++)
            interop_colour(ret, kk, i.first, i.second);
    }

    vec3f value_col = {100, 206, 209};

    interop_colour_string(ret, value_col);
    interop_colour_numbers(ret, value_col);
}
