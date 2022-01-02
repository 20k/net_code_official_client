#include "string_helpers.hpp"
#include "auto_handlers.hpp"

std::string escape_str(const std::string& in)
{
    std::string str = in;

    for(int i=0; i < (int)str.size(); i++)
    {
        if(str[i] == '\"')
        {
            str.insert(str.begin() + i, '\\');
            i++;
            continue;
        }
    }

    return str;
}

std::string escape_str(std::string_view in)
{
    return escape_str(std::string(in.begin(), in.end()));
}
