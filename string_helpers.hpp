#ifndef STRING_HELPERS_HPP_INCLUDED
#define STRING_HELPERS_HPP_INCLUDED

#include "colour_interop.hpp"
#include <string_view>

namespace char_inf
{
    inline int cwidth = 8;
    inline int cheight = 16;
    inline int cwbuf = 4;

    inline int font_size = 12;

    inline float extra_glyph_spacing = 0;
}

struct auto_handler;

std::string escape_str(const std::string& in);
std::string escape_str(std::string_view in);

#endif // STRING_HELPERS_HPP_INCLUDED
