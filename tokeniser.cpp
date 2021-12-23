#include "colour_interop.hpp"
#include "tokeniser.hpp"
#include <optional>
#include <libncclient/nc_util.hpp>
#include <iostream>

using data_t = const std::vector<interop_char>&;
using token_seq = std::vector<token_info>&;

template<typename Seq>
bool in_bound(int pos, const Seq& dat)
{
    return pos < (int)dat.size();
}


static char index(const std::vector<interop_char>& dat, int idx)
{
    return dat[idx].c;
}

static char index(std::string_view view, int idx)
{
    return view[idx];
}

template<typename Seq>
bool expect_seq(int pos, const Seq& dat, const std::string& str)
{
    for(int i=0; i < (int)str.size(); i++)
    {
        int offset = i + pos;

        if(!in_bound(offset, dat))
            return false;

        if(index(dat, offset) != str[i])
            return false;
    }

    return true;
}

enum expect_until_modes
{
    expect_until_do_none = 0,
    expect_until_do_eof = 1,
    expect_until_do_escape = 2,
    expect_until_is_not_name = 4,
    expect_until_allow_numeric_operators_in_name = 8,
};

bool is_numeric_operator(char c)
{
    return c == '-' || c == '+';
}

template<typename Seq>
std::optional<int> expect_until(int pos, const Seq& dat, const std::vector<char>& c, expect_until_modes mode = expect_until_do_none)
{
    bool escaped = false;

    for(int i=pos; i < (int)dat.size(); i++)
    {
        if((mode & expect_until_do_escape) > 0)
        {
            if(!escaped && index(dat, i) == '\\')
                escaped = true;
            else
                escaped = false;

            if(escaped)
            {
                i++;
                continue;
            }
        }

        if((mode & expect_until_is_not_name) > 0)
        {
            bool valid_operator = false;

            if((mode & expect_until_allow_numeric_operators_in_name) > 0)
            {
                valid_operator = is_numeric_operator(index(dat, i));
            }

            if(i == pos && (!is_valid_name_character(index(dat, i), true) && !valid_operator))
                return std::nullopt;

            if(i != pos && (!is_valid_name_character(index(dat, i), true) && !valid_operator))
            {
                return i;
            }
        }

        for(auto& kk : c)
        {
            if(index(dat, i) == kk)
                return i;
        }

        if(index(dat, i) == '\n')
            return std::nullopt;
    }

    if((mode & expect_until_do_eof) > 0)
    {
        return (int)dat.size();
    }

    return std::nullopt;
}

template<typename Seq>
bool is_whitespace(int pos, const Seq& dat)
{
    return isspace(index(dat, pos)) && index(dat, pos) != '\n';
}

template<typename Seq>
void discard_whitespace(int& pos, const Seq& dat, token_seq tok)
{
    while(in_bound(pos, dat) && is_whitespace(pos, dat))
    {
        pos++;
    }
}

template<typename Seq>
token_info make_tokens(int start, int length, token::token type, const Seq& dat)
{
    token_info ret;
    ret.type = type;
    ret.start_pos = start;
    ret.end_pos = start + length;

    for(int i=start; i < start + length && i < (int)dat.size(); i++)
    {
        ret.str.push_back(index(dat, i));
    }

    return ret;
}

token_info make_ghost_token(int start, token::token type, const std::string& chars)
{
    token_info ret;
    ret.type = type;
    ret.start_pos = start;
    ret.str = chars;
    ret.ghost = true;

    return ret;
}

template<typename Seq>
bool expect_dot(int& pos, const Seq& dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    if(index(dat, pos) == '.')
    {
        tok.push_back(make_tokens(pos, 1, token::DOT, dat));
        pos++;

        return true;
    }

    return false;
}

template<typename Seq>
bool expect_single_char(int& pos, const Seq& dat, token_seq tok, char c, token::token type, bool insert_ghosts, int ghost_offset)
{
    if(!in_bound(pos, dat))
    {
        if(insert_ghosts)
            tok.push_back(make_ghost_token(pos + ghost_offset, type, std::string(1, c)));

        return false;
    }

    if(index(dat, pos) == c)
    {
        tok.push_back(make_tokens(pos, 1, type, dat));
        pos++;

        return true;
    }

    if(insert_ghosts)
    {
        tok.push_back(make_ghost_token(pos + ghost_offset, type, std::string(1, c)));
    }

    return false;
}

///don't expect keys which are strings
template<typename Seq>
bool expect_key(int& pos, const Seq& dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    ///lots of these cases here are recoverable if we have autocompletes for the value,
    ///eg #script.name({key, -> #script.name({key:"", val:""})
    std::optional<int> found = expect_until(pos, dat, {'\"', '(', '{', ')', '}', ';', ':', ',', ' '}, expect_until_do_eof);

    if(!found.has_value())
        return false;

    auto fpos = *found;
    int len = fpos - pos;

    if(len <= 0)
        return false;

    tok.push_back(make_tokens(pos, len, token::KEY, dat));
    pos += len;

    return true;
}

template<typename Seq>
bool expect_value(int& pos, const Seq& dat, token_seq tok, bool insert_ghosts, int ghost_offset, bool lax_value_strictness, bool is_arrs = true)
{
    if(!in_bound(pos, dat))
        return false;

    bool is_string = false;

    if(index(dat, pos) == '\"' || index(dat, pos) == '\'')
    {
        is_string = true;
    }

    bool is_arr = false;

    if(index(dat, pos) == '[')
        is_arr = true;

    std::optional<int> found;

    token::token_subtype subtype = token::NONE;

    int start_pos = pos;

    if(is_string)
    {
        char start_c = index(dat, pos);

        found = expect_until(pos+1, dat, {start_c}, expect_until_do_escape);

        if(found.has_value())
        {
            *found = (*found) + 1;
            subtype = token::STRING;
        }
        else
        {
            //if(insert_ghosts)
            {
                ///if lax value strictness
                ///and insert ghosts
                ///should look for }) sequence and stop there if its at the end of the line
                ///bit of a hack but should work 99.9% of the time
                ///just check if dat.back and dat.back-1 are the correct characters
                ///and test index is > dat.back-1
                ///and if it is set it to be before back-1
                auto test = expect_until(pos+1, dat, {'\n'}, expect_until_do_eof);

                if(test.has_value() && lax_value_strictness && (int)dat.size() > 2)
                {
                    int len = dat.size();

                    ///input ends with })
                    if((index(dat, len-1) == ')' && index(dat, len-2) == '}') || (index(dat, len-1) == ')' && index(dat, len-2) == ']'))
                    {
                        *test = len-2;
                    }
                }

                if(insert_ghosts && test.has_value())
                {
                    auto token = make_ghost_token(*test + ghost_offset, token::QUOTE, std::string(1, start_c));

                    tok.push_back(token);
                }

                found = test;
            }

            //found = expect_until(pos+1, dat, {}, (expect_until_modes)(expect_until_do_eof | expect_until_is_not_name));
        }
    }
    else if(is_arr && expect_single_char(pos, dat, tok, '[', token::OPEN_SQUARE, false, 0))
    {
        discard_whitespace(pos, dat, tok);
        expect_value(pos, dat, tok, insert_ghosts, ghost_offset, lax_value_strictness);

        while(expect_single_char(pos, dat, tok, ',', token::COMMA, false, 0))
        {
            discard_whitespace(pos, dat, tok);
            expect_value(pos, dat, tok, insert_ghosts, ghost_offset, lax_value_strictness);
        }

        discard_whitespace(pos, dat, tok);
        expect_single_char(pos, dat, tok, ']', token::CLOSE_SQUARE, insert_ghosts, ghost_offset);

        found = pos;

        pos = start_pos;
    }
    else
    {
        ///HANDLE NON STRING CASE HERE
        //found = expect_until(pos, dat, {')', '}', ';', ':', ',', ' '}, expect_until_do_eof);

        if(!lax_value_strictness)
            found = expect_until(pos, dat, {}, (expect_until_modes)(expect_until_do_eof | expect_until_is_not_name | expect_until_allow_numeric_operators_in_name));
        else
            found = expect_until(pos, dat, {')', '}', ';', ':', ',', ' ', ']'}, expect_until_do_eof);

        ///validate number properly
        if(found.has_value())
        {
            subtype = token::GENERIC;

            bool all_numeric = true;

            for(int i=pos; i < *found; i++)
            {
                char c = index(dat, i);

                if(!isdigit(c) && c != '-' && c != '+')
                {
                    all_numeric = false;
                    break;
                }
            }

            if(all_numeric)
            {
                subtype = token::NUMBER;
            }

            std::vector<std::string> booleans
            {
                "true",
                "false",
            };

            std::string fstr;

            for(int i=pos; i < *found; i++)
            {
                fstr += index(dat, i);
            }

            for(auto& i : booleans)
            {
                if(i == fstr)
                {
                    subtype = token::BOOLEAN;
                    break;
                }
            }
        }
    }

    if(!found.has_value())
        return false;

    auto fpos = *found;
    int len = fpos - pos;

    if(len == 0)
        return false;

    auto token = make_tokens(pos, len, token::VALUE, dat);
    token.subtype = subtype;

    tok.push_back(token);
    pos += len;

    return true;
}

template<typename Seq>
bool expect_extname(int& pos, const Seq& dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    std::optional<int> found = expect_until(pos, dat, {'\"', '(', '{', ')', '}', ';', ' ', '\n'}, expect_until_do_eof);

    if(!found.has_value())
        return false;

    auto fpos = *found;
    int len = fpos - pos;

    tok.push_back(make_tokens(pos, len, token::EXT_NAME, dat));
    pos += len;

    return true;
}

template<typename Seq>
bool expect_hostname(int& pos, const Seq& dat, token_seq tok)
{
    std::optional<int> found = expect_until(pos, dat, {'.'});

    if(!found.has_value())
        return false;

    int dot_pos = *found;
    int str_len = dot_pos - pos;

    tok.push_back(make_tokens(pos, str_len, token::HOST_NAME, dat));
    pos += str_len;

    return true;
}

template<typename Seq>
bool expect_hash(int& pos, const Seq& dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    std::string expected = "#";

    if(expect_seq(pos, dat, expected))
    {
        tok.push_back(make_tokens(pos, expected.size(), token::HASH, dat));
        pos += expected.size();

        return true;
    }

    return false;
}

template<typename Seq>
bool expect_key_value(int& pos, const Seq& dat, token_seq tok, bool insert_ghosts, bool lax_value_strictness)
{
    bool success = true;

    success &= expect_key(pos, dat, tok);
    discard_whitespace(pos, dat, tok);

    expect_single_char(pos, dat, tok, ':', token::COLON, success, 0);
    discard_whitespace(pos, dat, tok);

    //std::cout << "fnd df " << found << std::endl;

    success &= expect_value(pos, dat, tok, insert_ghosts, 0, lax_value_strictness);
    discard_whitespace(pos, dat, tok);

    return success;
}

template<typename Seq>
bool expect_seclevel(int& pos, const Seq& dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    std::vector<std::string> match
    {
        "#fs.",
        "#hs.",
        "#ms.",
        "#ls.",
        "#ns.",
        "#s.",
        "#4s.",
        "#3s.",
        "#2s.",
        "#1s.",
        "#0s."
    };

    if(index(dat, pos) != '#')
        return false;

    std::string found;
    bool any = false;

    for(auto& sec : match)
    {
        bool all = true;

        for(int kk = 0; kk < (int)sec.size(); kk++)
        {
            int offset = kk + pos;

            if(offset >= (int)dat.size())
            {
                all = false;
                break;
            }

            if(sec[kk] != index(dat, offset))
            {
                all = false;
                break;
            }
        }

        if(!all)
            continue;

        found = sec;

        any = true;
        break;
    }

    if(!any && index(dat, pos) == '#')
    {
        found = "#";
        any = true;
    }

    if(!any)
        return false;

    tok.push_back(make_tokens(pos, 1, token::HASH, dat));
    pos++;

    //if(found != "#")
    {
        int tlen = ((int)found.size()) - 1;

        if(tlen < 0)
            tlen = 0;

        tok.push_back(make_tokens(pos, tlen, token::SECLEVEL, dat));
        pos += tlen;
    }

    return true;
}

template<typename Seq>
void tokenise_function_internal(int& pos, const Seq& dat, token_seq tok, bool insert_ghosts, bool lax_value_strictness)
{
    expect_hostname(pos, dat, tok);
    expect_dot(pos, dat, tok);
    expect_extname(pos, dat, tok);
    discard_whitespace(pos, dat, tok);

    bool opening_paren = expect_single_char(pos, dat, tok, '(', token::OPEN_PAREN, insert_ghosts, 0);
    discard_whitespace(pos, dat, tok);

    //bool array_style = expect_single_char(pos, dat, tok, '[', token::OPEN_SQUARE, false, 0);
    //discard_whitespace(pos, dat, tok);

    bool array_style = false;

    if(pos < (int)dat.size())
    {
        array_style = index(dat, pos) == '[';
    }

    bool success = true;
    bool opening_bracket = false;

    if(!array_style)
    {
        opening_bracket = expect_single_char(pos, dat, tok, '{', token::OPEN_CURLEY, insert_ghosts, 0);
    }

    discard_whitespace(pos, dat, tok);

    if(pos < (int)dat.size())
    {
        if(index(dat, pos) == '[')
            success = expect_value(pos, dat, tok, insert_ghosts, 0, lax_value_strictness);
        else
            success = expect_key_value(pos, dat, tok, insert_ghosts, lax_value_strictness);

        while(expect_single_char(pos, dat, tok, ',', token::COMMA, false, 0))
        {
            discard_whitespace(pos, dat, tok);

            if(pos < (int)dat.size())
            {
                if(index(dat, pos) == '[')
                    success = expect_value(pos, dat, tok, insert_ghosts, 0, lax_value_strictness);
                else
                    success = expect_key_value(pos, dat, tok, insert_ghosts, lax_value_strictness);
            }
        }
    }

    bool suppress = false;

    if(tok.size() > 0 && tok.back().ghost && (tok.back().type != token::OPEN_PAREN && tok.back().type != token::OPEN_CURLEY && tok.back().type != token::OPEN_SQUARE))
    {
        suppress = true;
    }


    if(!array_style)
    {
        expect_single_char(pos, dat, tok, '}', token::CLOSE_CURLEY, (success || opening_bracket || true) && !suppress, 0);
    }
    else
    {
        //if(!success)
        //    expect_single_char(pos, dat, tok, ']', token::CLOSE_SQUARE, !suppress, 0);
    }

    discard_whitespace(pos, dat, tok);

    expect_single_char(pos, dat, tok, ')', token::CLOSE_PAREN, (success || opening_paren) && !suppress, 0);
    discard_whitespace(pos, dat, tok);
}

template<typename Seq>
std::vector<token_info> tokenise_function_impl(const Seq& dat, bool insert_ghosts)
{
    std::vector<token_info> tok;

    int pos = 0;

    discard_whitespace(pos, dat, tok);

    //if(expect_hash(pos, dat, tok))
    if(expect_seclevel(pos, dat, tok))
    {
        tokenise_function_internal(pos, dat, tok, insert_ghosts, true);
    }

    return tok;
}

template<typename Seq>
std::vector<token_info> tokenise_general_impl(const Seq& dat)
{
    std::vector<token_info> tok;
    int pos = 0;

    for(; pos < (int)dat.size();)
    {
        bool any = false;

        /*while(dat[pos].c == '\n' && pos < (int)dat.size())
        {
            pos++;
        }*/

        discard_whitespace(pos, dat, tok);

        if(expect_seclevel(pos, dat, tok))
        {
            tokenise_function_internal(pos, dat, tok, false, false);
            any |= true;
            continue;
        }

        any |= expect_value(pos, dat, tok, false, 0, false);

        any |= expect_single_char(pos, dat, tok, '(', token::OPEN_PAREN, false, 0);
        any |= expect_single_char(pos, dat, tok, ')', token::CLOSE_PAREN, false, 0);
        any |= expect_single_char(pos, dat, tok, '{', token::OPEN_CURLEY, false, 0);
        any |= expect_single_char(pos, dat, tok, '}', token::CLOSE_CURLEY, false, 0);
        any |= expect_single_char(pos, dat, tok, '[', token::OPEN_SQUARE, false, 0);
        any |= expect_single_char(pos, dat, tok, ']', token::CLOSE_SQUARE, false, 0);

        if(!any)
            pos++;
    }

    return tok;
}

std::vector<token_info> tokenise_function(const std::vector<interop_char>& dat, bool insert_ghosts)
{
    return tokenise_function_impl(dat, insert_ghosts);
}

std::vector<token_info> tokenise_general(const std::vector<interop_char>& dat)
{
    return tokenise_general_impl(dat);
}

std::vector<token_info> tokenise_function(std::string_view dat, bool insert_ghosts)
{
    return tokenise_function_impl(dat, insert_ghosts);
}

std::vector<token_info> tokenise_general(std::string_view dat)
{
    return tokenise_general_impl(dat);
}

bool has_token(token::token type, const std::vector<token_info>& tokens)
{
    for(auto& i : tokens)
    {
        if(type == i.type)
            return true;
    }

    return false;
}

token_info get_token(token::token type, const std::vector<token_info>& tokens)
{
    token_info ret;

    for(auto& i : tokens)
    {
        if(type == i.type)
            return i;
    }

    return ret;
}

std::string tokens_to_full_script(const std::vector<token_info>& tokens)
{
    bool valid = has_token(token::HOST_NAME, tokens) && has_token(token::DOT, tokens) && has_token(token::EXT_NAME, tokens);

    if(!valid)
        return "";

    return get_token(token::HOST_NAME, tokens).str + get_token(token::DOT, tokens).str + get_token(token::EXT_NAME, tokens).str;
}

void token_tests()
{
    #define T0
    #ifdef T0
    std::cout << "token testing\n";

    std::string base_str = "#scripts.core({user:\"hello\", doot:\"doot\",asdf:93});";

    std::vector<interop_char> chars = build_from_colour_string(base_str, false);

    std::vector<token_info> tokens = tokenise_function(chars, true);

    std::vector<token::token> expected
    {
        token::HASH,
        token::SECLEVEL,
        token::HOST_NAME,
        token::DOT,
        token::EXT_NAME,
        token::OPEN_PAREN,
        token::OPEN_CURLEY,
        token::KEY,
        token::COLON,
        token::VALUE,
        token::COMMA,
        token::KEY,
        token::COLON,
        token::VALUE,
        token::COMMA,
        token::KEY,
        token::COLON,
        token::VALUE,
        token::CLOSE_CURLEY,
        token::CLOSE_PAREN,
        token::SEMICOLON,
    };

    std::cout << "found " << tokens.size() << " tokens" << std::endl;

    for(int i=0; i < (int)tokens.size(); i++)
    {
        if(tokens[i].type != expected[i])
        {
            printf("failure at %i %i %i %s\n", i, tokens[i].type, expected[i], tokens[i].str.c_str());
        }
        else
        {
            printf("success at %i %i %i %s\n", i, tokens[i].type, expected[i], tokens[i].str.c_str());
        }
    }
    #endif // T0

    //#define T1
    #ifdef T1
    std::string base_str = "#scripts.core({user:\"hello\", doot:\"doot\",asdf:93, NID:-1, asdf:\"hi\"});";

    std::vector<interop_char> chars = build_from_colour_string(base_str, false);

    std::vector<token_info> tokens = tokenise_function(chars, true);

    for(int i=0; i < (int)tokens.size(); i++)
    {
        printf("success at %i %i %s\n", i, tokens[i].type, tokens[i].str.c_str());
    }

    #endif // T1

    return;
}
