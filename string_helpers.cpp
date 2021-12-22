#include "string_helpers.hpp"
#include "auto_handlers.hpp"

interop_vec_t string_to_interop(const std::string& str, bool render_specials, auto_handler& auto_handle, bool parse_for_autocompletes)
{
    interop_vec_t chars = build_from_colour_string(str, render_specials);

    if(str.size() < 2000)
        auto_handle.auto_colour(chars, false, parse_for_autocompletes);

    return chars;
}

interop_vec_t string_to_interop_no_autos(const std::string& str, bool render_specials)
{
    return build_from_colour_string(str, render_specials);
}

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
