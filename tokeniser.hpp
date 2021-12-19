#ifndef TOKENISER_HPP_INCLUDED
#define TOKENISER_HPP_INCLUDED

#include <vector>
#include <string>
#include <string_view>

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
        OPEN_SQUARE = 14,
        CLOSE_SQUARE = 15,
        QUOTE = 16,
        SPACE = 17,
        ERR,
    };

    enum token_subtype
    {
        NUMBER,
        STRING,
        GENERIC,
        KEYWORD,
        BOOLEAN,
        NONE,
    };
}

struct token_info
{
    token::token type = token::ERR;
    token::token_subtype subtype = token::NONE;

    std::string str;
    int start_pos = 0;
    int end_pos = 0;

    bool ghost = false;
};

std::vector<token_info> tokenise_function(const std::vector<interop_char>& ichars, bool insert_ghosts);
std::vector<token_info> tokenise_general(const std::vector<interop_char>& ichars);

std::vector<token_info> tokenise_function(std::string_view ichars, bool insert_ghosts);
std::vector<token_info> tokenise_general(std::string_view ichars);

std::string tokens_to_full_script(const std::vector<token_info>& tokens);

token_info make_ghost_token(int start, token::token type, const std::string& chars);

void token_tests();

#endif // TOKENISER_HPP_INCLUDED
