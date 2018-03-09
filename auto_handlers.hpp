#ifndef AUTO_HANDLERS_HPP_INCLUDED
#define AUTO_HANDLERS_HPP_INCLUDED

#include <vector>

struct interop_char;

struct auto_handler
{
    void auto_colour(std::vector<interop_char>& in, bool colour_special = false);
};

#endif // AUTO_HANDLERS_HPP_INCLUDED
