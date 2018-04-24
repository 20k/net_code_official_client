#include "auto_handlers.hpp"
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include <SFML/System.hpp>
#include "util.hpp"
#include "tokeniser.hpp"

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

inline
void colour_interop(std::vector<interop_char>& in, int start, int fin, vec3f col)
{
    for(int kk=start; kk < fin; kk++)
    {
        if(kk < 0)
            continue;
        if(kk >= (int)in.size())
            return;

        in[kk].col = col;
    }
}

void auto_handler::auto_colour(std::vector<interop_char>& in, bool colour_special, bool parse_for_autocompletes)
{
    std::map<std::string, vec3f> cols
    {
        {"fs.", {60, 255, 60}},
        {"hs.", {255, 255, 40}},
        {"ms.", {255, 140, 40}},
        {"ls.", {255, 20, 20}},
        {"ns.", {255, 20, 255}},
        {"s.", {255, 20, 255}},
        {"", {255, 20, 255}},
    };

    vec3f pale_blue = {120, 120, 255};
    vec3f pale_red = {255, 60, 60};

    cols["{"] = pale_red;
    cols["}"] = pale_red;
    cols["["] = pale_red;
    cols["]"] = pale_red;

    std::map<std::string, vec3f> generic_keywords;

    if(use_autocolour)
    {
        generic_keywords["function"] = pale_blue;
        generic_keywords["while"] = pale_blue;
        generic_keywords["for"] = pale_blue;
        generic_keywords["if"] = pale_blue;
        generic_keywords["return"] = pale_blue;
        generic_keywords[";"] = pale_red;
    }

    std::set<token::token> valid_colourings
    {
        token::SECLEVEL,
        //token::OPEN_PAREN,
        token::OPEN_CURLEY,
        //token::CLOSE_PAREN,
        token::CLOSE_CURLEY,
        token::OPEN_SQUARE,
        token::CLOSE_SQUARE,
    };

    vec3f value_col = {100, 206, 209};

    for(int i=0; i < (int)in.size(); i++)
    {
        std::vector<token_info> tokens = tokenise_general(in);

        for(token_info& i : tokens)
        {
            if(valid_colourings.find(i.type) != valid_colourings.end())
            {
                colour_interop(in, i.start_pos, i.end_pos, cols[i.str]);
            }
            else if(i.type == token::VALUE && (i.subtype == token::STRING || i.subtype == token::NUMBER))
            {
                if(use_autocolour || i.subtype == token::NUMBER)
                    colour_interop(in, i.start_pos, i.end_pos, value_col);
            }
            else if(use_autocolour && i.type == token::VALUE && i.subtype == token::GENERIC && use_autocolour)
            {
                for(auto& ss : generic_keywords)
                {
                    if(ss.first == i.str)
                    {
                        colour_interop(in, i.start_pos, i.end_pos, ss.second);
                        break;
                    }
                }
            }
        }

        if(parse_for_autocompletes)
        {
            for(int kk=0; kk < (int)tokens.size() - 3; kk++)
            {
                if(tokens[kk].type == token::HOST_NAME && tokens[kk+1].type == token::DOT && tokens[kk+2].type == token::EXT_NAME)
                {
                    std::string str = tokens[kk].str + "." + tokens[kk+2].str;

                    bool exists = found_args.find(str) != found_args.end();

                    if(str.size() != 0 && !exists)
                    {
                        //std::cout << "fauto " << str << std::endl;

                        found_unprocessed_autocompletes.insert(str);
                    }
                }
            }

            /*for(auto& kk : tokens)
            {
                std::cout << "toke " << kk.str << " type " << kk.type << std::endl;
            }*/
        }

        for(auto& kk : tokens)
        {
            i += kk.str.size();
        }
    }
}

void insert_kv_ghosts(const std::vector<std::string>& keys, const std::vector<std::string>& vals, int pos, std::vector<token_info>& tokens, std::vector<interop_char>& in, int num_concrete_args)
{
    ///should be completely impossible
    if(pos < 0)
        return;

    int char_pos = in.size();

    if(pos < (int)tokens.size())
        char_pos = tokens[pos].start_pos;

    for(int i=0; i < (int)keys.size(); i++)
    {
        std::string key = keys[i];
        std::string val = vals[i];

        if(num_concrete_args > 0)
        {
            ///it says comma
            ///but some sort of space ghost has gotten in the works
            tokens.insert(tokens.begin() + pos++, make_ghost_token(char_pos, token::COMMA, ", "));
        }

        token_info arg_token = make_ghost_token(char_pos, token::KEY, key);
        token_info col_token = make_ghost_token(char_pos, token::COLON, ":");
        token_info val_token = make_ghost_token(char_pos, token::VALUE, val);
        val_token.subtype = token::GENERIC;

        tokens.insert(tokens.begin() + pos++, arg_token);
        tokens.insert(tokens.begin() + pos++, col_token);
        tokens.insert(tokens.begin() + pos++, val_token);

        if(i != (int)keys.size()-1)
        {
            tokens.insert(tokens.begin() + pos++, make_ghost_token(char_pos, token::COMMA, ", "));
        }
    }
}

void insert_single_ghost(const std::string& str, int pos, std::vector<token_info>& tokens, std::vector<interop_char>& in, int ghost_offset)
{
    if(pos < 0)
        return;

    int char_pos = in.size();

    if(pos < (int)tokens.size())
        char_pos = tokens[pos].start_pos - 1 + ghost_offset;

    token_info val_token = make_ghost_token(char_pos, token::VALUE, str);

    tokens.insert(tokens.begin() + pos, val_token);
}

    /*auto it = found_args.upper_bound(name);

    if(it == found_args.end() || !starts_with(it->first, name))
    {
        if(tab_pressed)
        {
            command_str += "()";
            cursor_idx = command_str.size();
        }

        return false;
    }

    auto splits = no_ss_split(name, ".");

    if(splits.size() != 2)
        return false;

    std::string start = name;

    auto pair_its = std::mismatch(name.begin(), name.end(), it->first.begin(), it->first.end());
    auto distance = std::distance(name.begin(), pair_its.first);

    if(pair_its.second == it->first.end())
        return false;

    std::string str(pair_its.second, it->first.end());*/

//bool auto_handler::handle_script_autocomplete(std::vector<interop_char>& in, int& cursor_idx, std::string& command_str, const std::string& name);


void auto_handler::handle_autocompletes(std::vector<interop_char>& in, int& cursor_idx, int& cursor_offset, std::string& command_str)
{
    std::vector<token_info> tokens = tokenise_function(in, true);

    std::string script_name = tokens_to_full_script(tokens);

    if(script_name == "")
        return;

    auto it = found_args.upper_bound(script_name);

    if(it == found_args.end() || !starts_with(it->first, script_name))
    {
        if(tab_pressed && command_str.size() != 0 && command_str.back() != ')')
        {
            command_str += "()";

            in = string_to_interop_no_autos(command_str, false);

            cursor_idx = in.size();
        }

        return;
    }

    std::string ghost_str;

    if(found_args.find(script_name) == found_args.end())
    {
        auto pair_its = std::mismatch(script_name.begin(), script_name.end(), it->first.begin(), it->first.end());

        if(pair_its.second != it->first.end())
        {
            ghost_str = std::string(pair_its.second, it->first.end());
            //std::cout << "ghost " << ghost_str << " fullname " << script_name << " lookup " << it->first << std::endl;
        }
    }

    for(auto& i : tokens)
    {
        if(i.type == token::OPEN_PAREN || i.type == token::CLOSE_PAREN || i.type == token::OPEN_CURLEY || i.type == token::CLOSE_CURLEY || i.type == token::SEMICOLON)
        {
            if(i.ghost)
                continue;

            ghost_str = "";
        }
    }

    std::map<std::string, std::string> key_to_arg;

    std::map<std::string, bool> key_exists_map;
    std::map<std::string, bool> value_exists_map; ///so we can type user: + [tab] -> user:"|"

    for(int i=0; i<(int)tokens.size(); i++)
    {
        if(tokens[i].type == token::KEY)
        {
            key_exists_map[tokens[i].str] = true;
        }
    }

    std::vector<autocomplete_args> my_args;

    if(found_args.find(script_name) != found_args.end())
        my_args = found_args[script_name];

    for(auto& i : my_args)
    {
        key_to_arg[i.key] = i.arg;
    }

    int num_real_args = 0;

    std::map<std::string, bool> should_ghost;

    for(auto& i : my_args)
    {
        should_ghost[i.key] = true;
    }

    for(auto& i : key_exists_map)
    {
        should_ghost[i.first] = false;
        num_real_args++;
    }

    std::vector<std::string> keys;
    std::vector<std::string> vals;

    for(auto& i : should_ghost)
    {
        if(!i.second)
            continue;

        keys.push_back(i.first);
        vals.push_back(key_to_arg[i.first]);
    }

    for(int i=0; i < (int)tokens.size(); i++)
    {
        token_info& tok = tokens[i];

        if(tok.type == token::EXT_NAME && ghost_str != "")
        {
            insert_single_ghost(ghost_str, i+1, tokens, in, 1);

            break;
        }

        if(tok.type == token::CLOSE_PAREN)
        {
            insert_kv_ghosts(keys, vals, i, tokens, in, num_real_args);

            break;
        }

        if(tok.type == token::CLOSE_CURLEY)
        {
            insert_kv_ghosts(keys, vals, i, tokens, in, num_real_args);

            break;
        }
    }

    ///ok. now we need to do some parsing of the tokens themselves
    ///need to insert autocomplete args into the tokens
    ///as well as detect args we already have there

    ///core goals:
    ///pressing tab inserts one arg at a time and moves cursor there
    ///after autocompleting a string jumps into string
    ///after autocompleting a value jumps to the end of the value
    ///pressing tab cycles between args

    cursor_offset = 0;

    int in_offset = 0;
    int no_col_offset = 0;

    for(int i=0; i < (int)tokens.size(); i++)
    {
        token_info& tok = tokens[i];

        if(tok.ghost)
        {
            std::string ex_str = "`c" + tok.str + "`";

            auto interop = string_to_interop_no_autos(ex_str, false);

            int full_offset = tok.start_pos + in_offset;
            int full_no_col_offset = tok.start_pos + no_col_offset;

            if(full_offset > (int)in.size())
                full_offset = in.size();

            if(full_no_col_offset > (int)command_str.size())
                full_no_col_offset = command_str.size();

            in.insert(in.begin() + full_offset, interop.begin(), interop.end());

            if(tab_pressed)
            {
                command_str.insert(command_str.begin() + full_no_col_offset, tok.str.begin(), tok.str.end());
            }

            in_offset += interop.size();
            no_col_offset += tok.str.size();

            if(tok.start_pos < cursor_idx)
            {
                cursor_offset++;
            }
        }
    }

    if(tab_pressed)
    {
        cursor_offset = 0;
        cursor_idx = in.size();
    }
}

#if 0
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

bool until(const std::vector<interop_char>& t, int& idx, int min_len, int max_len, const std::vector<std::string>& c, bool must_be_alpha = true, bool allow_eof = false)
{
    int len = 0;

    int start = idx;

    while(idx < (int)t.size() && len < max_len && is_any_of(c, t, idx) == -1)
    {
        if(allow_eof && idx >= (int)t.size())
            break;

        if(must_be_alpha && !isalnum(t[idx].c) && t[idx].c != '_' && is_any_of(c, t, idx) == -1)
        {
            idx = start;
            return false;
        }

        len++;

        idx++;
    }

    if(allow_eof && idx >= (int)t.size())
        return true;

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
int get_autocomplete(std::vector<interop_char>& chs, int idx, std::string& out, bool& null_terminated, bool allow_extended = false)
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

    if(!until(chs, idx, 1, MAX_ANY_NAME_LEN, valid_terminators, true, true))
        return 0;

    int mval = std::min(idx, (int)chs.size());
    idx = mval;

    for(int i=start; i < idx && i < (int)chs.size(); i++)
    {
        out.push_back(chs[i].c);
    }

    int rlen = (idx - start_full_length) + 1;

    if(rlen <= 0)
        return 0;


    if(idx < chs.size())
    {
        null_terminated = false;
        return rlen;
    }
    else
    {
        null_terminated = true;
        return rlen - 1;
    }
}

void auto_handler::auto_colour(std::vector<interop_char>& ret, bool colour_special, bool parse_for_autocompletes)
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
    if(parse_for_autocompletes)
    {
        for(int i=0; i < (int)ret.size(); i++)
        {
            std::string out;

            bool null_terminated = false;

            get_autocomplete(ret, i, out, null_terminated, true);

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

bool auto_handler::handle_script_autocomplete(std::vector<interop_char>& in, int& cursor_idx, std::string& command_str, const std::string& name)
{
    if(found_args.find(name) != found_args.end())
        return false;

    ///cannot be equal, but is lexigdfdfasfd the closest match above us
    ///longer is considered above
    auto it = found_args.upper_bound(name);

    if(it == found_args.end() || !starts_with(it->first, name))
    {
        if(tab_pressed)
        {
            command_str += "()";
            cursor_idx = command_str.size();
        }

        return false;
    }

    auto splits = no_ss_split(name, ".");

    if(splits.size() != 2)
        return false;

    std::string start = name;

    auto pair_its = std::mismatch(name.begin(), name.end(), it->first.begin(), it->first.end());
    auto distance = std::distance(name.begin(), pair_its.first);

    if(pair_its.second == it->first.end())
        return false;

    /*auto found_it = it->first.find(".");
    int d2 = std::distance()*/

    std::string str(pair_its.second, it->first.end());

    if(!tab_pressed)
    {
        str = "`c" + str + "`";

        auto interop = string_to_interop_no_autos(str, false);
        in.insert(in.end(), interop.begin(), interop.end());
    }
    else
    {
        command_str += str;
        cursor_idx = command_str.size();
    }

    return true;
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

    bool null_terminated = false;

    for(int i=0; i < (int)in.size(); i++)
    {
        std::string out;

        bool fnull = false;

        int len = get_autocomplete(in, i, out, fnull, true);

        //if(null_terminated)
        //    std::cout << "hello\n";

        bool exists = found_args.find(out) != found_args.end();

        if(exists || (out.size() > 0 && fnull))
        {
            found = out;
            where = i;
            flen = len;
            null_terminated = fnull;
        }
    }

    if(found.size() == 0)
        return;

    if(where < 0)
        return;

    //if(null_terminated != 1)
    //std::cout << "yay " << null_terminated << std::endl;

    //if(in.size() != flen)
    //    return;

    if(in.size() == 0)
        return;

    if(in.back().c == ')' || in.back().c == ';')
        return;

    if(null_terminated)
    {
        ///we return because its too complex to faff about with the parser atm
        if(handle_script_autocomplete(in, cursor_idx, command_str, found))
            return;
    }

    bool exists = found_args.find(found) != found_args.end();

    if(!exists)
        return;

    //if(is_valid.find(found) == is_valid.end() || is_valid[found] == false)
    //    return;

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

    std::vector<autocomplete_args> args;

    if(found_args.find(found) != found_args.end())
    {
        args = found_args[found];
    }

    //std::cout << "asize " << args.size() << std::endl;

    {
        handle_tab(in, cursor_idx, parse_start, args, specials, command_str, to_skip);
    }

    ///we need to skip constructs here if they're already inserted
    std::string str;

    if(!specials.has_open_paren)
        str += "`c(`";

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
    if(!tab_pressed)
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
        parse_start++;
        //return;
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
        if(!specials.has_close_curly)
            command_str += "}";
        if(!specials.has_close_paren)
            command_str += ")";

        cursor_idx = command_str.size();

        return;
    }

    ///has open curly
}

void auto_handler::clear_internal_state()
{
    internal_state = 0;
}
#endif // 0

void auto_handler::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(found_unprocessed_autocompletes, ser);
    s.handle_serialise(found_args, ser);
    s.handle_serialise(is_valid, ser);
}
