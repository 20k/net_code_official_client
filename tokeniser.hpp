#ifndef TOKENISER_HPP_INCLUDED
#define TOKENISER_HPP_INCLUDED

#include <vector>
#include <string>

struct interop_char;

namespace token
{
    enum token
    {
        HASH = 0,
        SECLEVEL = 1,
        HOST_NAME = 2,
        DOT = 3,
        EXT_NAME = 4,
        OPEN_PAREN = 5,
        OPEN_CURLEY = 6,
        KEY = 7,
        COLON = 8,
        VALUE = 9,
        COMMA = 10,
        CLOSE_PAREN = 11,
        CLOSE_CURLEY = 12,
        SEMICOLON = 13, ///terminating
        ERR,
    };
}


struct token_info
{
    token::token type = token::ERR;

    std::string str;
    int start_pos = 0;
    int end_pos = 0;

    bool ghost = false;
};

std::vector<token_info> tokenise_str(const std::vector<interop_char>& ichars, bool insert_ghosts);

void token_tests();

#endif // TOKENISER_HPP_INCLUDED
