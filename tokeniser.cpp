#include "colour_interop.hpp"
#include "tokeniser.hpp"
#include <optional>

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

std::optional<int> expect_until(int pos, data_t dat, const std::vector<char>& c, bool allow_eof = false)
{
    for(int i=pos; i < (int)dat.size(); i++)
    {
        for(auto& kk : c)
        {
            if(dat[i].c == kk)
                return i;
        }
    }

    if(allow_eof)
    {
        return (int)dat.size();
    }

    return std::nullopt;
}

bool is_whitespace(int pos, data_t dat)
{
    return isspace(dat[pos].c);
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

bool expect_extname(int& pos, data_t dat, token_seq tok)
{
    if(!in_bound(pos, dat))
        return false;

    std::optional<int> found = expect_until(pos, dat, {'\"', '(', '{', ')', '}', ';'}, true);

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

std::vector<token_info> tokenise_str(const std::vector<interop_char>& dat)
{
    std::vector<token_info> tok;

    int pos = 0;

    discard_whitespace(pos, dat, tok);

    if(expect_hash(pos, dat, tok))
    {
        expect_hostname(pos, dat, tok);
        expect_dot(pos, dat, tok);
        expect_extname(pos, dat, tok);
    }

    return tok;
}

void token_tests()
{
    std::cout << "token testing\n";

    std::string base_str = "#scripts.core({user:\"hello\"});";

    std::vector<interop_char> chars = build_from_colour_string(base_str, false);

    std::vector<token_info> tokens = tokenise_str(chars);

    std::vector<token::token> expected
    {
        token::HASH,
        token::HOST_NAME,
        token::DOT,
        token::EXT_NAME,
        token::OPEN_PAREN,
        token::OPEN_CURLEY,
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
            printf("failure at %i\n", i);
        }
        else
        {
            printf("success at %i %s\n", i, tokens[i].str.c_str());
        }
    }

    return;
}
