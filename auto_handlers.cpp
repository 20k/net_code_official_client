#include "auto_handlers.hpp"
#include "string_helpers.hpp"
#include <crapmud/script_util_shared.hpp>
#include <SFML/System.hpp>
#include "util.hpp"

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

    for(int i=idx; i < idx + (int)op.size() && i < (int)chs.size(); i++)
    {
        if(chs[i].coloured)
            continue;

        chs[i].col = col;
    }
}

template<typename T>
int is_any_of(const T& t, const std::vector<interop_char>& c, int idx)
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

bool until(const std::vector<interop_char>& t, int& idx, int min_len, int max_len, const std::vector<std::string>& c, bool must_be_alpha = true)
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

    if(which >= (int)c.size())
    {
        idx = start;
        return false;
    }

    idx += (int)c[which].size();

    return true;
}

void remove_whitespace(std::vector<interop_char>& t, int& idx)
{
    while(idx < (int)t.size() && t[idx].c == ' ')
        idx++;
}

///out is the name of the autocomplete to add. If the input is #fs.script.name, we get script.name
///returns the full length of #fs.script.name
int get_autocomplete(std::vector<interop_char>& chs, int idx, std::string& out, bool allow_extended = false)
{
    out = std::string();

    if(idx >= (int)chs.size())
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

    std::vector<std::string> valid_terminators = {";", "(", " ", "\n"};

    ///basically a hack to let #scripts.core() work
    if(allow_extended)
    {
        valid_terminators.push_back("\"");
    }

    if(!until(chs, idx, 1, MAX_ANY_NAME_LEN, {";", "(", " ", "\n"}))
        return 0;

    for(int i=start; i < idx && i < (int)chs.size(); i++)
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

    cols["{"] = pale_red;
    cols["}"] = pale_red;
    cols["["] = pale_red;
    cols["]"] = pale_red;

    //if(colour_special)
    if(use_autocolour)
    {
        cols["function?"] = pale_blue;
        cols["while?"] = pale_blue;
        cols["for?"] = pale_blue;
        cols["if?"] = pale_blue;
        cols["return?"] = pale_blue;
        cols[";"] = pale_red;
    }

    for(auto& i : cols)
    {
        for(int kk=0; kk < (int)ret.size(); kk++)
            interop_colour(ret, kk, i.first, i.second);
    }

    vec3f value_col = {100, 206, 209};

    interop_colour_numbers(ret, value_col);

    if(!use_autocolour)
        return;

    interop_colour_string(ret, value_col);

    ///find full strings to autocomplete
    ///uses different parsing algorithm to is_valid
    {
        for(int i=0; i < (int)ret.size(); i++)
        {
            std::string out;

            get_autocomplete(ret, i, out, true);

            bool exists = found_args.find(out) != found_args.end();

            if(out.size() != 0 && !exists)
            {
                found_unprocessed_autocompletes.insert(out);
            }
        }
    }
}

std::vector<std::string> get_skip_argnames(std::vector<interop_char>& in, int parse_start, specials_status& specials)
{
    ///so
    ///the pattern we're looking for is either
    ///{ arg:
    ///or
    ///, arg:

    std::vector<std::string> ret;

    int idx = parse_start - 1;

    if(expect(in, idx, {"("}))
    {
        specials.has_open_paren = true;
    }

    while(idx < (int)in.size())
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
            specials.has_open_curly = true;

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

        ret.push_back(arg);
    }

    {
        int check_trailing = idx;

        while(check_trailing >= 0 && in[check_trailing].c == ' ')
            check_trailing--;

        if(check_trailing >= 0 && in[check_trailing].c == ',')
            specials.has_trailing_comma = true;
    }

    if(!until(in, idx, 0, MAX_ANY_NAME_LEN, {"}"}, false))
        return ret;

    if(in[idx].c == '}')
        specials.has_close_curly = true;

    if(!until(in, idx, 0, MAX_ANY_NAME_LEN, {")"}, false))
        return ret;

    if(in[idx].c == ')')
        specials.has_close_paren = true;

    return ret;
}

///so. We should store an arg offset
///then when tab happens, insert the first arg properly
///bump the arg offset
///when the command is sent, we need to bump the offset back to 0
void auto_handler::handle_autocompletes(std::vector<interop_char>& in, int& cursor_idx, std::string& command_str)
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

    specials_status specials;

    int parse_start = flen + where;

    auto to_skip = get_skip_argnames(in, parse_start, specials);

    std::vector<autocomplete_args> args = found_args[found];

    {
        handle_tab(in, cursor_idx, parse_start, args, specials, command_str, to_skip);
    }

    ///we need to skip constructs here if they're already inserted
    std::string str;

    //if(!specials.has_open_paren)
    //    str += "`c(";

    if(!specials.has_open_curly)
        str += "`c{`";

    /*for(auto& i : args)
    {
        str += "`F" + i.key + "`:`P" + i.arg + "``c, `";
    }*/

    //for(auto& i : args)

    bool had_last = false;

    for(int i=0; i < (int)args.size(); i++)
    {
        const auto& p = args[i];

        if(std::find(to_skip.begin(), to_skip.end(), p.key) != to_skip.end())
            continue;

        if(had_last || (!specials.has_trailing_comma && to_skip.size() > 0))
            str += "`c, `";

        str += "`c" + p.key + ":" + p.arg + "`";

        had_last = true;
    }

    ///ok so
    ///we have a list of args
    ///what we want to do is: on tab, cycle to next arg
    ///regardless of the autocomplete stuff? But also including the autocomplete string
    ///which means: need to parse current actual text, as well as autocompletes (which we have in the form of to skips)
    ///all to skips must occur before all autocompletes (due to the nature of this)
    ///which means that when we tab, we first cycle through the to skips, then we cycle through the autocompletes
    ///when cycling into an autocomplete, we need to make it 'real' in the code and place the cusor just past the : i think
    ///don't want to get more swanky than that atm

    if(!specials.has_close_curly)
        str += "`c}`";

    if(!specials.has_close_paren)
        str += "`c)`";

    auto interop = string_to_interop_no_autos(str, false);

    in.insert(in.end(), interop.begin(), interop.end());

    //in.insert(in.begin() + flen + where, interop.begin(), interop.end());
}

void auto_handler::handle_tab(const std::vector<interop_char>& in, int& cursor_idx, int parse_start, const std::vector<autocomplete_args>& found, const specials_status& specials, std::string& command_str, const std::vector<std::string>& to_skip)
{
    if(!ONCE_MACRO(sf::Keyboard::Tab) || !window_in_focus)
        return;

    ///so. We've pressed tab
    ///we start at cursor_idx
    ///we look for the next : symbol and jump to it. Check if we have a ({ construct, and insert { if necessary
    ///if we find none, we check through the autocompletes
    ///and then insert the next. Need to check commas
    ///if there are none left and we press tab, insert })

    if(!specials.has_open_paren && command_str.size() > 0)
    {
        if(!isalnum(command_str.back()))
        {
            parse_start--;
            command_str.pop_back();
        }

        command_str.insert(command_str.begin() + parse_start, '(');

        cursor_idx = command_str.size();
        return;
    }

    if(!specials.has_open_curly)
    {
        command_str.insert(command_str.begin() + parse_start, '{');
        cursor_idx = command_str.size();
        return;
    }

    ///insert new funtimes, found no args
    if(!until(in, cursor_idx, 0, MAX_ANY_NAME_LEN, {":"}, false))
    {
        if(found.size() > 0)
        {
            for(auto& to_insert : found)
            {
                if(std::find(to_skip.begin(), to_skip.end(), to_insert.key) != to_skip.end())
                    continue;

                std::string str = to_insert.key + ":" + to_insert.arg;

                if(!specials.has_trailing_comma && to_skip.size() > 0)
                {
                    str = ", " + str;
                }

                command_str += str;

                cursor_idx += str.size();

                return;
            }

        }
    }

    ///nothing left to insert
    if(!specials.has_close_curly || !specials.has_close_paren)
    {
        command_str += "})";
        cursor_idx = command_str.size();

        return;
    }

    /*cursor_idx++;

    if(cursor_idx >= (int)command_str.size())
        return;*/



    ///has open curly
}

void auto_handler::clear_internal_state()
{
    internal_state = 0;
}
