#ifndef AUTO_HANDLERS_HPP_INCLUDED
#define AUTO_HANDLERS_HPP_INCLUDED

#include <vector>

struct interop_char;

struct auto_handler
{
    std::vector<interop_char> auto_colour(const std::vector<interop_char>& in, bool colour_special = false);
};

#endif // AUTO_HANDLERS_HPP_INCLUDED
