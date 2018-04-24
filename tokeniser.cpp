#include "colour_interop.hpp"
#include "tokeniser.hpp"

std::vector<token_info> tokenise_str(const std::vector<interop_char>& ichars)
{
    std::vector<token_info> tokens;


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

    for(int i=0; i < tokens.size(); i++)
    {
        if(tokens[i].type != expected[i])
        {
            printf("failure at %i\n", i);
        }
    }

    return;
}
