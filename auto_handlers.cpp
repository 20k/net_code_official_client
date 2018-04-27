#include "auto_handlers.hpp"
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include <SFML/System.hpp>
#include "util.hpp"
#include "tokeniser.hpp"

inline
void colour_interop(std::vector<interop_char>& in, int start, int fin, vec3f col)
{
    for(int kk=start; kk < fin; kk++)
    {
        if(kk < 0)
            continue;
        if(kk >= (int)in.size())
            return;

        if(in[kk].coloured)
            continue;

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
        }

        for(auto& kk : tokens)
        {
            i += kk.str.size();
        }
    }
}

int insert_kv_ghosts(const std::vector<std::string>& keys, const std::vector<std::string>& vals, int pos, std::vector<token_info>& tokens, std::vector<interop_char>& in, int num_concrete_args)
{
    ///should be completely impossible
    if(pos < 0)
        return 0;

    int char_pos = in.size();

    if(pos > (int)tokens.size())
        pos = tokens.size();

    if(pos < (int)tokens.size())
        char_pos = tokens[pos].start_pos;

    bool insert_front = num_concrete_args > 0;
    bool insert_space = false;

    int lpos = pos - 1;

    if(lpos >= 0)
    {
        if(tokens[lpos].type == token::COMMA)
        {
            insert_front = false;

            token_info& ctoken = tokens[lpos];
            int spos = ctoken.start_pos + 1;

            if(ctoken.str.size() == 1 && ((spos < (int)in.size() && in[spos].c != ' ') || spos >= (int)in.size()))
                insert_space = true;
        }
    }

    int num = 0;

    for(int i=0; i < (int)keys.size(); i++)
    {
        std::string key = keys[i];
        std::string val = vals[i];

        if(insert_front)
        {
            ///it says comma
            ///but some sort of space ghost has gotten in the works
            tokens.insert(tokens.begin() + pos++, make_ghost_token(char_pos, token::COMMA, ", "));

            num++;

            insert_front = false;
        }

        ///INSERT SPACE GHOST
        if(insert_space)
        {
            tokens.insert(tokens.begin() + pos++, make_ghost_token(char_pos, token::SPACE, " "));

            num++;

            insert_space = false;
        }

        token_info arg_token = make_ghost_token(char_pos, token::KEY, key);
        token_info col_token = make_ghost_token(char_pos, token::COLON, ":");
        token_info val_token = make_ghost_token(char_pos, token::VALUE, val);
        val_token.subtype = token::GENERIC;

        if(val.size() >= 2)
        {
            auto tokenise_temp = tokenise_general(string_to_interop_no_autos(val, true));

            if(tokenise_temp.size() > 0 && tokenise_temp[0].type == token::VALUE && tokenise_temp[0].subtype == token::STRING)
            {
                val_token.subtype = token::STRING;
            }
        }

        tokens.insert(tokens.begin() + pos++, arg_token);
        tokens.insert(tokens.begin() + pos++, col_token);
        tokens.insert(tokens.begin() + pos++, val_token);

        num += 3;

        if(i != (int)keys.size()-1)
        {
            tokens.insert(tokens.begin() + pos++, make_ghost_token(char_pos, token::COMMA, ", "));

            num++;
        }
    }

    return num;
}

int insert_single_ghost(const std::string& str, int pos, std::vector<token_info>& tokens, std::vector<interop_char>& in, int ghost_offset)
{
    if(pos < 0)
        return 0;

    int num = 0;

    int char_pos = in.size();

    if(pos < (int)tokens.size())
        char_pos = tokens[pos].start_pos - 1 + ghost_offset;

    token_info val_token = make_ghost_token(char_pos, token::EXT_NAME, str);

    tokens.insert(tokens.begin() + pos, val_token);
    num++;

    return num;
}

void handle_tab_cycling(auto_handler& auto_handle, const std::vector<token_info>& tokens, std::vector<interop_char>& in, int& cursor_idx, int& cursor_offset)
{
    if(!auto_handle.tab_pressed)
        return;

    cursor_offset = 0;

    std::vector<int> key_starts;
    std::vector<int> key_mvs;
    std::vector<int> is_string;

    for(int i=0; i < (int)tokens.size(); i++)
    {
        const token_info& i1 = tokens[i];

        if(i1.type == token::VALUE)
        {
            key_starts.push_back(i1.start_pos);
            key_mvs.push_back(i1.end_pos);
            is_string.push_back(i1.subtype == token::STRING);
        }
    }

    for(int i=0; i < (int)key_starts.size(); i++)
    {
        int next = (i + 1) % (int)key_starts.size();

        if(cursor_idx >= key_starts[i] && cursor_idx < key_starts[next])
        {
            cursor_idx = key_mvs[next] - is_string[next];
            break;
        }

        if(cursor_idx >= key_starts[i] && next < i)
        {
            cursor_idx = key_mvs[next] - is_string[next];
            break;
        }

        if(i == 0 && cursor_idx < key_starts[i])
        {
            cursor_idx = key_mvs[i] - is_string[i];
            break;
        }
    }
}

void auto_handler::handle_autocompletes(std::vector<interop_char>& in, int& cursor_idx, int& cursor_offset, std::string& command_str)
{
    std::vector<token_info> tokens = tokenise_function(in, true);

    std::string script_name = tokens_to_full_script(tokens);

    if(script_name == "")
        return;

    auto it = found_args.upper_bound(script_name);

    if((it == found_args.end() || !starts_with(it->first, script_name)) && found_args.find(script_name) == found_args.end())
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
            break;
        }
    }

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
    {
        my_args = found_args[script_name];
    }
    else if(found_args.find(script_name + ghost_str) != found_args.end())
    {
        my_args = found_args[script_name + ghost_str];
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

    for(auto& arg : my_args)
    {
        if(!should_ghost[arg.key])
            continue;

        keys.push_back(arg.key);
        vals.push_back(arg.arg);
    }

    /*if(keys.size() > 1)
    {
        keys.resize(1);
        vals.resize(1);
    }*/

    bool once_insert = false;

    for(int i=0; i < (int)tokens.size(); i++)
    {
        token_info& tok = tokens[i];

        if(tok.type == token::EXT_NAME && ghost_str != "")
        {
            int add = insert_single_ghost(ghost_str, i+1, tokens, in, 1);

            i += add;
            continue;
        }

        if(tok.type == token::CLOSE_PAREN && !once_insert)
        {
            int add = insert_kv_ghosts(keys, vals, i, tokens, in, num_real_args);

            i += add;
            once_insert = true;
            continue;
        }

        if(tok.type == token::CLOSE_CURLEY && !once_insert)
        {
            int add = insert_kv_ghosts(keys, vals, i, tokens, in, num_real_args);

            i += add;
            once_insert = true;
            continue;
        }
    }

    //std::cout << "once " << once_insert << std::endl;

    if(!once_insert)
    {
        insert_kv_ghosts(keys, vals, 999, tokens, in, num_real_args);
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

    bool keep_inserting = true;

    int inserted_to = -1;
    bool value_inserted = false;

    bool any_ghosts_inserted = false;

    for(int i=0; i < (int)tokens.size(); i++)
    {
        token_info& tok = tokens[i];

        if(tok.ghost)
        {
            any_ghosts_inserted = true;

            std::string ex_str = "`c" + tok.str + "`";

            auto interop = string_to_interop_no_autos(ex_str, false);

            int full_offset = tok.start_pos + in_offset;
            int full_no_col_offset = tok.start_pos + no_col_offset;

            if(full_offset > (int)in.size())
                full_offset = in.size();

            if(full_no_col_offset > (int)command_str.size())
                full_no_col_offset = command_str.size();

            in.insert(in.begin() + full_offset, interop.begin(), interop.end());

            if(tab_pressed && (keep_inserting || tok.type == token::CLOSE_PAREN || tok.type == token::CLOSE_CURLEY || tok.type == token::OPEN_CURLEY || tok.type == token::OPEN_PAREN))
            {
                command_str.insert(command_str.begin() + full_no_col_offset, tok.str.begin(), tok.str.end());

                if(!value_inserted && keep_inserting)
                    inserted_to = full_no_col_offset + tok.str.size();

                if(tok.type == token::VALUE && tok.subtype == token::STRING && !value_inserted)
                {
                    inserted_to--;
                }

                if(tok.type == token::VALUE)
                    value_inserted = true;
            }

            if(tok.type == token::VALUE)
                keep_inserting = false;

            if(tok.type == token::OPEN_CURLEY)
                keep_inserting = false;

            in_offset += interop.size();
            no_col_offset += tok.str.size();

            if(tok.start_pos < cursor_idx)
            {
                cursor_offset += tok.str.size();
            }
        }
    }

    if(inserted_to == -1)
        inserted_to = in.size();

    if(tab_pressed)
    {
        if(any_ghosts_inserted)
        {
            cursor_offset = 0;
            //cursor_idx = in.size();

            if(inserted_to > (int)in.size())
                inserted_to = in.size();

            cursor_idx = inserted_to;
        }
        else
        {
            handle_tab_cycling(*this, tokens, in, cursor_idx, cursor_offset);
        }
    }
}

void auto_handler::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(found_unprocessed_autocompletes, ser);
    s.handle_serialise(found_args, ser);
    s.handle_serialise(is_valid, ser);
}
