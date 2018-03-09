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
    for(int i=start; i < fin; i++)
    {
        chs[i].col = col;
        chs[i].coloured = true;
    }
}

void interop_colour_numbers(std::vector<interop_char>& chs, vec3f col)
{
    for(int i=0; i < (int)chs.size(); i++)
    {
        if(isdigit(chs[i].c) && !chs[i].coloured)
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

void interop_colour(std::vector<interop_char>& chs, int idx, std::string str, vec3f col)
{
    if(!interop_starts_with(chs, idx, str))
        return;

    strip_input(str);

    for(int i=idx; i < idx + (int)str.size(); i++)
    {
        if(chs[i].coloured)
            continue;

        chs[i].col = col;
    }
}

std::vector<interop_char> auto_handler::auto_colour(const std::vector<interop_char>& in, bool colour_special)
{
    std::vector<interop_char> ret = in;

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

    interop_colour_numbers(ret, {255, 60, 255});

    return ret;
}
