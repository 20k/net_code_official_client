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
        HOST_NAME = 1,
        DOT = 2,
        EXT_NAME = 3,
        OPEN_PAREN = 4,
        OPEN_CURLEY = 5,
        KEY = 6,
        COLON = 7,
        VALUE = 8,
        COMMA = 9,
        CLOSE_PAREN = 10,
        CLOSE_CURLEY = 11,
        SEMICOLON = 12, ///terminating
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
