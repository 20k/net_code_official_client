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

    //std::cout << "fidx " << idx << std::endl;
    //std::cout << "flen " << len << std::endl;

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

void remove_whitespace(std::vector<interop_char>& t, int& idx)
{
    while((int)idx < t.size() && t[idx].c == ' ')
        idx++;
}

///out is the name of the autocomplete to add. If the input is #fs.script.name, we get script.name
///returns the full length of #fs.script.name
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

    int start_full_length = idx;

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

    ///the +1 is to include the end character
    return (idx - start_full_length) + 1;
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

            get_autocomplete(ret, i, out);

            bool exists = found_args.find(out) != found_args.end();

            if(out.size() != 0 && !exists)
            {
                found_unprocessed_autocompletes.insert(out);
            }
        }
    }
}

std::set<std::string> get_skip_argnames(std::vector<interop_char>& in, int parse_start, bool& opening_curly, bool& closing_curly, bool& closing_paren)
{
    ///so
    ///the pattern we're looking for is either
    ///{ arg:
    ///or
    ///, arg:

    std::set<std::string> ret;

    int idx = parse_start;

    while(idx < in.size())
    {
        //std::cout << "hi" << std::endl;

        remove_whitespace(in, idx);

        std::vector<interop_char> fnd(in.begin() + idx, in.end());

        /*for(auto& i : fnd)
        {
            std::cout << i.c;
        }

        std::cout << std::endl;*/

        ///find first instance of "{" or ","
        ///ideally we'd parse away strings
        if(!until(in, idx, 0, MAX_ANY_NAME_LEN, {"{", ","}, false))
            break;

        if(in[idx].c == '{')
            opening_curly = true;

        //std::cout << "expected" << std::endl;

        idx++;

        remove_whitespace(in, idx);

        int cur = idx;

        ///find the : symbol
        if(!until(in, idx, 0, MAX_ANY_NAME_LEN, {":"}, false))
            break;

        //std::cout << "e2 " << std::endl;

        ///this is our arg
        //std::string arg(in.begin() + cur, in.begin() + idx);

        std::string arg;

        for(auto it = in.begin() + cur; it != in.begin() + idx; it++)
        {
            arg += it->c;
        }

        //std::cout << "arg " << arg << std::endl;

        ret.insert(arg);
    }

    if(!until(in, idx, 0, MAX_ANY_NAME_LEN, {"}"}, false))
        return ret;

    if(in[idx].c == '}')
        closing_curly = true;

    if(!until(in, idx, 0, MAX_ANY_NAME_LEN, {")"}, false))
        return ret;

    if(in[idx].c == ')')
        closing_paren = true;

    return ret;
}

///so. We should store an arg offset
///then when tab happens, insert the first arg properly
///bump the arg offset
///when the command is sent, we need to bump the offset back to 0
void auto_handler::handle_autocompletes(std::vector<interop_char>& in, int& cursor_idx)
{
    if(!use_autocomplete)
        return;

    ///in is disposable
    ///aka we freely edit it

    std::string found;
    int where = -1;
    int flen = -1;

    for(int i=0; i < (int)in.size(); i++)
    {
        std::string out;

        int len = get_autocomplete(in, i, out);

        bool exists = found_args.find(out) != found_args.end();

        if(exists)
        {
            found = out;
            where = i;
            flen = len;
        }
    }

    if(found.size() == 0)
        return;

    if(where < 0)
        return;

    //if(in.size() != flen)
    //    return;

    if(in.size() == 0)
        return;

    if(in.back().c == ')' || in.back().c == ';')
        return;

    ///ok
    ///Do i really need to parse arg string?
    ///need to extract key params I think
    ///to see what args we've inserted
    ///and then skip those args
    ///so. Look for name: and name :
    ///and then skippity those args

    bool has_open_curly = false;
    bool has_close_curly = false;
    bool has_close_paren = false;

    int parse_start = flen + where;

    auto to_skip = get_skip_argnames(in, parse_start, has_open_curly, has_close_curly, has_close_paren);

    std::vector<autocomplete_args> args = found_args[found];

    ///we need to skip constructs here if they're already inserted
    std::string str;

    if(!has_open_curly)
        str += "`c{`";

    /*for(auto& i : args)
    {
        str += "`F" + i.key + "`:`P" + i.arg + "``c, `";
    }*/

    //for(auto& i : args)

    bool had_last = false;

    for(int i=0; i < args.size(); i++)
    {
        const auto& p = args[i];

        if(to_skip.find(p.key) != to_skip.end())
            continue;

        if(had_last)
            str += "`c, `";

        str += "`c" + p.key + ":" + p.arg + "`";

        had_last = true;
    }

    if(!has_close_curly)
        str += "`c}`";

    if(!has_close_paren)
        str += "`c)`";

    auto interop = string_to_interop_no_autos(str, false);

    in.insert(in.end(), interop.begin(), interop.end());

    //in.insert(in.begin() + flen + where, interop.begin(), interop.end());
}

void auto_handler::clear_internal_state()
{
    internal_state = 0;
}
