#ifndef TOKENISER_HPP_INCLUDED
#define TOKENISER_HPP_INCLUDED

#include <vector>
#include <string>

struct interop_char;

namespace token
{
    enum token
    {
        HASH,
        HOST_NAME,
        DOT,
        EXT_NAME,
        OPEN_PAREN,
        OPEN_CURLEY,
        KEY,
        COLON,
        VALUE,
        COMMA,
        CLOSE_PAREN,
        CLOSE_CURLEY,
        SEMICOLON, ///terminating
        ERR,
    };
}


struct token_info
{
    token::token type = token::ERR;

    std::string str;
    int start_pos = 0;
    int end_pos = 0;
};

std::vector<token_info> tokenise_str(const std::vector<interop_char>& ichars);

void token_tests();

#endif // TOKENISER_HPP_INCLUDED
