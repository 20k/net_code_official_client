#include "colour_interop.hpp"
#include "tokeniser.hpp"
#include <optional>
#include <libncclient/nc_util.hpp>

using data_t = const std::vector<interop_char>&;
using token_seq = std::vector<token_info>&;

bool in_bound(int pos, data_t dat)
{
    return pos < (int)dat.size();
}

bool expect_seq(int pos, data_t dat, const std::string& str)
{
    for(int i=0; i < (int)str.size(); i++)
    {
        int offset = i + pos;

        if(!in_bound(offset, dat))
            return false;

        if(dat[offset].c != str[i])
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
};

std::optional<int> expect_until(int pos, data_t dat, const std::vector<char>& c, expect_until_modes mode = expect_until_do_none)
{
    bool escaped = false;

    for(int i=pos; i < (int)dat.size(); i++)
    {
        if((mode & expect_until_do_escape) > 0)
        {
            if(!escaped && dat[i].c == '\\')
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
            if(i == pos && !is_valid_name_character(dat[i].c, true))
                return std::nullopt;

            if(i != pos && !is_valid_name_character(dat[i].c, true))
            {
                return i;
            }
        }

        for(auto& kk : c)
        {
            if(dat[i].c == kk)
                return i;
        }

        if(dat[i].c == '\n')
            return std::nullopt;
    }

    if((mode & expect_until_do_eof) > 0)
    {
        return (int)dat.size();
    }

    return std::nullopt;
}

bool is_whitespace(int pos, data_t dat)
{
    return isspace(dat[pos].c) && dat[pos].c != '\n';
}

void discard_whitespace(int& pos, data_t dat, token_seq tok)
{
    while(in_bound(pos, dat) && is_whitespace(pos, dat))
    {
        pos++;
    }
}

token_info make_tokens(int start, int length, token::token type, data_t dat)
{
    token_info ret;
    ret.type = type;
    ret.start_pos = start;
    ret.end_pos = start + length;

    for(int i=start; i < start + length && i < (int)dat.size(); i++)
    {
        ret.str.push_back(dat[i].c);
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

bool expect_dot(int& pos, data_t dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    if(dat[pos].c == '.')
    {
        tok.push_back(make_tokens(pos, 1, token::DOT, dat));
        pos++;

        return true;
    }

    return false;
}

bool expect_single_char(int& pos, data_t dat, token_seq tok, char c, token::token type, bool insert_ghosts, int ghost_offset)
{
    if(!in_bound(pos, dat))
    {
        if(insert_ghosts)
            tok.push_back(make_ghost_token(pos + ghost_offset, type, std::string(1, c)));

        return false;
    }

    if(dat[pos].c == c)
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
bool expect_key(int& pos, data_t dat, token_seq tok)
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

    tok.push_back(make_tokens(pos, len, token::KEY, dat));
    pos += len;

    return true;
}

bool expect_value(int& pos, data_t dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    bool is_string = false;

    if(dat[pos].c == '\"' || dat[pos].c == '\'')
    {
        is_string = true;
    }

    std::optional<int> found;

    token::token_subtype subtype = token::NONE;

    if(is_string)
    {
        char start_c = dat[pos].c;

        found = expect_until(pos+1, dat, {start_c}, expect_until_do_escape);

        if(found.has_value())
        {
            *found = (*found) + 1;
            subtype = token::STRING;
        }
    }
    else
    {
        ///HANDLE NON STRING CASE HERE
        //found = expect_until(pos, dat, {')', '}', ';', ':', ',', ' '}, expect_until_do_eof);

        found = expect_until(pos, dat, {}, (expect_until_modes)(expect_until_do_eof | expect_until_is_not_name));

        ///validate number properly
        if(found.has_value())
        {
            subtype = token::GENERIC;

            bool all_numeric = true;

            for(int i=pos; i < *found; i++)
            {
                if(!isdigit(dat[i].c))
                {
                    all_numeric = false;
                    break;
                }
            }

            if(all_numeric)
            {
                subtype = token::NUMBER;
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

bool expect_extname(int& pos, data_t dat, token_seq tok)
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

bool expect_hostname(int& pos, data_t dat, token_seq tok)
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

bool expect_hash(int& pos, data_t dat, token_seq tok)
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

bool expect_key_value(int& pos, data_t dat, token_seq tok, bool insert_ghosts)
{
    bool success = true;

    success &= expect_key(pos, dat, tok);
    discard_whitespace(pos, dat, tok);

    expect_single_char(pos, dat, tok, ':', token::COLON, success, 0);
    discard_whitespace(pos, dat, tok);

    //std::cout << "fnd df " << found << std::endl;

    success &= expect_value(pos, dat, tok);
    discard_whitespace(pos, dat, tok);

    return success;
}

bool expect_seclevel(int& pos, data_t dat, token_seq tok)
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
    };

    if(dat[pos].c != '#')
        return false;

    std::string found;
    bool any = false;

    for(auto& i : match)
    {
        bool all = true;

        for(int kk = 0; kk < (int)i.size(); kk++)
        {
            int offset = kk + pos;

            if(offset >= (int)dat.size())
            {
                all = false;
                break;
            }

            if(i[kk] != dat[offset].c)
            {
                all = false;
                break;
            }
        }

        if(!all)
            continue;

        found = i;

        any = true;
        break;
    }

    if(!any && dat[pos].c == '#')
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

void tokenise_function_internal(int& pos, data_t dat, token_seq tok, bool insert_ghosts)
{
    expect_hostname(pos, dat, tok);
    expect_dot(pos, dat, tok);
    expect_extname(pos, dat, tok);
    discard_whitespace(pos, dat, tok);

    expect_single_char(pos, dat, tok, '(', token::OPEN_PAREN, insert_ghosts, 0);
    discard_whitespace(pos, dat, tok);

    expect_single_char(pos, dat, tok, '{', token::OPEN_CURLEY, insert_ghosts, 0);
    discard_whitespace(pos, dat, tok);

    bool success = true;

    if(pos < dat.size())
    {
        success = expect_key_value(pos, dat, tok, insert_ghosts);

        while(expect_single_char(pos, dat, tok, ',', token::COMMA, false, 0))
        {
            discard_whitespace(pos, dat, tok);
            success = expect_key_value(pos, dat, tok, insert_ghosts);
        }
    }

    //std::cout << "success " << success << std::endl;

    expect_single_char(pos, dat, tok, '}', token::CLOSE_CURLEY, success, 1);
    discard_whitespace(pos, dat, tok);

    expect_single_char(pos, dat, tok, ')', token::CLOSE_PAREN, success, 1);
    discard_whitespace(pos, dat, tok);
}

std::vector<token_info> tokenise_function(const std::vector<interop_char>& dat, bool insert_ghosts)
{
    std::vector<token_info> tok;

    int pos = 0;

    discard_whitespace(pos, dat, tok);

    //if(expect_hash(pos, dat, tok))
    if(expect_seclevel(pos, dat, tok))
    {
        tokenise_function_internal(pos, dat, tok, insert_ghosts);
    }

    return tok;
}

std::vector<token_info> tokenise_general(const std::vector<interop_char>& dat)
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
            tokenise_function_internal(pos, dat, tok, false);
            any |= true;
            continue;
        }

        any |= expect_value(pos, dat, tok);

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

    return;
}
