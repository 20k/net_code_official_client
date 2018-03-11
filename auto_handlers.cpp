#include "auto_handlers.hpp"
#include "string_helpers.hpp"
#include <crapmud/script_util_shared.hpp>

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


        if(ch == starter && (ch == "\"" || ch == "\'") && !escaped)
        {
            force_col(chs, start, i+1, col);
            starter = "";
            //continue;
        }

        escaped = false;
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

    for(int i=idx; i < idx + (int)op.size() && i < chs.size(); i++)
    {
        if(chs[i].coloured)
            continue;

        chs[i].col = col;
    }
}

template<typename T>
int is_any_of(const T& t, std::vector<interop_char>& c, int idx)
{
    //for(auto& str : t)
    for(int kk=0; kk < (int)t.size(); kk++)
    {
        auto str = t[kk];

        bool all = true;

        for(int i=0; i < (int)str.size(); i++)
        {
            int offset = idx + i;

            if(offset >= (int)c.size())
            {
                all = false;
                break;
            }

            if(str[i] != c[offset].c)
            {
                all = false;
                break;
            }
        }

        if(all)
        {
            return kk;
        }
    }

    return -1;
}

bool until(std::vector<interop_char>& t, int& idx, int min_len, int max_len, const std::vector<std::string>& c, bool must_be_alpha = true)
{
    int len = 0;

    int start = idx;

    while(idx < (int)t.size() && len < max_len && is_any_of(c, t, idx) == -1)
    {
        if(must_be_alpha && !isalnum(t[idx].c) && is_any_of(c, t, idx) == -1)
        {
            idx = start;
            return false;
        }

        len++;

        idx++;
    }

    if(idx >= (int)t.size())
    {
        idx = start;
        return false;
    }

    if(len >= max_len)
    {
        idx = start;
        return false;
    }

    if(len < min_len)
    {
        idx = start;
        return false;
    }

    //std::cout << "len " << len << " ml " << max_len << std::endl;

    return true;
}

bool expect(std::vector<interop_char>& t, int& idx, const std::vector<std::string>& c)
{
    int start = idx;

    int which = is_any_of(c, t, idx);

    if(which == -1)
        return false;

    if(which >= c.size())
    {
        idx = start;
        return false;
    }

    idx += c[which].size();

    return true;
}

int get_autocomplete(std::vector<interop_char>& chs, int idx, std::string& out)
{
    out = std::string();

    if(idx >= chs.size())
        return 0;

    if(chs[idx].c != '#')
        return 0;

    std::vector<std::string> match
    {
        "#fs.",
        "#hs.",
        "#ms.",
        "#ls.",
        "#ns.",
        "#s.",
    };

    if(!expect(chs, idx, match))
    {
        if(!expect(chs, idx, {"#"}))
            return 0;
    }

    int start = idx;

    ///#fs.[]

    ///#fs.namehere[.]
    if(!until(chs, idx, 1, MAX_ANY_NAME_LEN, {"."}))
        return 0;

    idx++;

    if(!until(chs, idx, 1, MAX_ANY_NAME_LEN, {";", "(", " ", "\n"}))
        return 0;

    for(int i=start; i < idx && i < chs.size(); i++)
    {
        out.push_back(chs[i].c);
    }

    //std::cout << "fnd " << out << std::endl;

    return idx - start;
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
        for(int kk=0; kk < (int)ret.size(); kk++)
            interop_colour(ret, kk, i.first, i.second);
    }

    vec3f value_col = {100, 206, 209};

    interop_colour_string(ret, value_col);
    interop_colour_numbers(ret, value_col);

    ///find full strings to autocomplete
    ///uses different parsing algorithm to is_valid
    {
        for(int i=0; i < (int)ret.size(); i++)
        {
            std::string out;

            int len = get_autocomplete(ret, i, out);

            if(len != 0 && found_args.find(out) == found_args.end())
            {
                found_unprocessed_autocompletes.insert(out);
            }
        }
    }
}
