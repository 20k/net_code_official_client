#include "auto_handlers.hpp"
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include "util.hpp"
#include "tokeniser.hpp"
#include "render_string.hpp"
#include <iostream>

/*token::HASH,
token::SECLEVEL,
token::HOST_NAME,
token::DOT,
token::EXT_NAME,*/

std::optional<vec3f> token_to_colour(const token_info& i, bool colour_like_terminal)
{
    std::map<std::string, vec3f> cols
    {
        {"fs.", {60, 255, 60}},
        {"hs.", {255, 255, 40}},
        {"ms.", {255, 140, 40}},
        {"ls.", {255, 20, 20}},
        {"ns.", {255, 20, 255}},
        {"4s.", {60, 255, 60}},
        {"3s.", {255, 255, 40}},
        {"2s.", {255, 140, 40}},
        {"1s.", {255, 20, 20}},
        {"0s.", {255, 20, 255}},
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

    if(colour_like_terminal)
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

    vec3f value_col = {8, 143, 242};
    //vec3f value_col = {100, 206, 209};
    vec3f key_col = {243, 166, 3};

    if(valid_colourings.count(i.type) > 0)
    {
        return cols[i.str];
    }

    else if(i.type == token::VALUE && (i.subtype == token::STRING || i.subtype == token::NUMBER || i.subtype == token::BOOLEAN))
    {
        if(colour_like_terminal || i.subtype == token::NUMBER || i.subtype == token::BOOLEAN)
        {
            return value_col;
        }
    }
    else if(colour_like_terminal && i.type == token::VALUE && i.subtype == token::GENERIC)
    {
        for(auto& ss : generic_keywords)
        {
            if(ss.first == i.str)
            {
                return ss.second;
            }
        }
    }
    else if(i.type == token::KEY)
    {
        return key_col;
    }
    else if(i.type == token::HOST_NAME)
    {
        return letter_to_colour(string_to_colour(i.str).at(0)).value();
    }

    return std::nullopt;
}

std::vector<render_string> auto_colour(std::string_view in, bool colour_like_terminal)
{
    vec3f default_colour = letter_to_colour('A').value();

    std::vector<render_string> strings;

    std::vector<token_info> tokens = tokenise_general(in);

    for(int i=0; i < (int)tokens.size(); i++)
    {
        token_info& tok = tokens[i];

        if(tok.start_pos == tok.end_pos || tok.ghost)
        {
            tokens.erase(tokens.begin() + i);
            i--;
            continue;
        }
    }

    if(tokens.size() > 0)
    {
        token_info& first = tokens.front();

        if(first.start_pos != 0)
        {
            render_string next;
            next.start =  0;
            next.length = first.start_pos;
            next.colour = srgb_to_lin(default_colour/255.f)*255.f;

            strings.push_back(next);
        }
    }

    for(int kk=0; kk < (int)tokens.size(); kk++)
    {
        token_info& i = tokens[kk];

        if(kk > 0)
        {
            token_info& last = tokens[kk - 1];

            if(last.end_pos != i.start_pos)
            {
                render_string next;
                next.start = last.end_pos;
                next.length = i.start_pos - last.end_pos;
                next.colour = srgb_to_lin(default_colour/255.f)*255.f;

                strings.push_back(next);
            }
        }

        int old_string_size = strings.size();

        auto add_coloured_string = [&](vec3f srgb_colour)
        {
            render_string next;
            next.start = i.start_pos;
            next.length = i.end_pos - i.start_pos;
            next.colour = srgb_to_lin(srgb_colour/255.f)*255.f;

            strings.push_back(next);
        };

        std::optional<vec3f> colour = token_to_colour(i, colour_like_terminal);

        if(colour.has_value())
        {
            add_coloured_string(colour.value());
        }

        if(old_string_size == (int)strings.size())
        {
            add_coloured_string(default_colour);
        }
    }

    if(tokens.size() > 0)
    {
        token_info& last = tokens.back();

        if(last.end_pos != (int)in.size())
        {
            render_string next;
            next.start = last.end_pos;
            next.length = (int)in.size() - last.end_pos;
            next.colour = srgb_to_lin(default_colour/255.f)*255.f;

            strings.push_back(next);
        }
    }

    if(tokens.size() == 0)
    {
        render_string next;
        next.start = 0;
        next.length = in.size();
        next.colour = srgb_to_lin(default_colour/255.f)*255.f;

        strings.push_back(next);
    }

    return strings;
}

std::vector<std::string> parse_for_autocompletes(std::string_view view)
{
    std::vector<std::string> found_unprocessed_autocompletes;

    std::vector<token_info> tokens = tokenise_general(view);

    for(int kk=0; kk < (int)tokens.size() - 3; kk++)
    {
        if(tokens[kk].type == token::HOST_NAME && tokens[kk+1].type == token::DOT && tokens[kk+2].type == token::EXT_NAME)
        {
            std::string str = tokens[kk].str + "." + tokens[kk+2].str;

            if(str.size() != 0)
            {
                found_unprocessed_autocompletes.push_back(std::move(str));
            }
        }
    }

    return found_unprocessed_autocompletes;
}

static char index(std::string_view view, int idx)
{
    return view[idx];
}

template<typename T>
int insert_kv_ghosts(const std::vector<std::string>& keys, const std::vector<std::string>& vals, int pos, std::vector<token_info>& tokens, const T& in, int num_concrete_args)
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

            if(ctoken.str.size() == 1 && ((spos < (int)in.size() && index(in, spos) != ' ') || spos >= (int)in.size()))
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
            auto tokenise_temp = tokenise_general(val);

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

template<typename T>
int insert_single_ghost(const std::string& str, int pos, std::vector<token_info>& tokens, T& in, int ghost_offset)
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

void handle_tab_cycling(auto_handler& auto_handle, const std::vector<token_info>& tokens, int& cursor_idx, int& cursor_offset)
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

    if(key_starts.size() == 0)
    {
        for(int i=0; i < (int)tokens.size(); i++)
        {
            const token_info& i1 = tokens[i];

            if(i1.type == token::CLOSE_CURLEY)
            {
                cursor_idx = i1.start_pos;
            }
        }
    }
}

void replace_string(std::string& in, std::string_view replacement)
{
    in = std::string(replacement.begin(), replacement.end());
}

std::string to_underlying_type(const std::string& tag, const std::string& str)
{
    return str;
}

void auto_handler::handle_autocompletes(std::string& in, int& cursor_idx, int& cursor_offset, std::string& command_str)
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

            replace_string(in, command_str);

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
        const token_info& tok = tokens[i];

        if(tok.ghost)
        {
            any_ghosts_inserted = true;

            std::string ex_str = "`|" + tok.str + "`";

            int full_offset = tok.start_pos + in_offset;
            int full_no_col_offset = tok.start_pos + no_col_offset;

            if(full_offset > (int)in.size())
                full_offset = in.size();

            if(full_no_col_offset > (int)command_str.size())
                full_no_col_offset = command_str.size();

            auto interop = to_underlying_type(in, ex_str);
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
            handle_tab_cycling(*this, tokens, cursor_idx, cursor_offset);
        }
    }

    return;
}

void auto_handler::extract_server_commands(nlohmann::json& in)
{
    std::string type = in["type"];

    if(type == "script_args")
    {
        std::string scriptname = in["script"];

        std::cout << scriptname << std::endl;

        if(scriptname.size() > 0)
        {
            std::vector<autocomplete_args> auto_args;

            for(int i=0; i < (int)in["keys"].size(); i++)
            {
                std::string key = in["keys"][i];
                std::string val = in["vals"][i];

                auto_args.push_back({key, val});
            }

            found_args[scriptname] = auto_args;
            is_valid[scriptname] = true;
        }
    }
    else if(type == "script_args_invalid")
    {
        std::string scriptname = in["script"];

        std::cout << "inv " << scriptname << std::endl;

        if(scriptname.size() > 0)
        {
            is_valid[scriptname] = false;
        }
    }
    else if(type == "script_args_ratelimit")
    {
        std::string name = in["script"];

        std::cout << "rl name " << name << std::endl;

        if(name.size() > 0)
        {
            found_unprocessed_autocompletes.push_back(name);
        }
    }
}

void auto_handler::make_server_request(connection_send_data& send)
{
    if(found_unprocessed_autocompletes.size() > 0 && time_since_last_request.get_elapsed_time_s() > 0.3)
    {
        time_since_last_request.restart();

        for(const std::string& str : found_unprocessed_autocompletes)
        {
            nlohmann::json data;
            data["type"] = "autocomplete_request";
            data["data"] = str;

            write_data dat;
            dat.id = -1;
            dat.data = data.dump();

            send.write_to_websocket(std::move(dat));

            break;
        }

        if(found_unprocessed_autocompletes.size() > 0)
            found_unprocessed_autocompletes.erase(found_unprocessed_autocompletes.begin());
    }
}
