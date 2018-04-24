#include "colour_interop.hpp"
#include "tokeniser.hpp"

using data_t = const std::vector<interop_char>&;
using token_seq = std::vector<token_info>&;

bool in_bound(int pos, data_t dat)
{
    return pos < dat.size();
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

void expect_hash(int& pos, data_t dat, token_seq tok)
{
    //discard_whitespace()

    std::string expected = "#";

    //if(dat)
}

std::vector<token_info> tokenise_str(const std::vector<interop_char>& ichars)
{
    std::vector<token_info> tokens;

    int pos = 0;

    discard_whitespace(pos, ichars, tokens);

    return tokens;
}

void token_tests()
{
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

    for(int i=0; i < (int)tokens.size(); i++)
    {
        if(tokens[i].type != expected[i])
        {
            printf("failure at %i\n", i);
        }
    }

    return;
}
