#ifndef FORMAT_CACHE_HPP_INCLUDED
#define FORMAT_CACHE_HPP_INCLUDED

#include "colour_interop.hpp"
#include "string_helpers.hpp"

struct scrollbar_hack;

struct format_cache
{
    int total_lines = 0;
    bool valid_cache = false;
    std::vector<std::vector<formatted_char>> cache;
    std::vector<std::vector<formatted_char>> out;

    void invalidate()
    {
        valid_cache = false;
    }

    void ensure_built(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns);

    std::vector<std::vector<formatted_char>> get_cache()
    {
        return cache;
    }

    int get_lines()
    {
        return total_lines;
    }

    bool valid()
    {
        return valid_cache;
    }
};

struct cacheable
{
    format_cache cache;

    std::vector<interop_vec_t> history;
};

#endif // FORMAT_CACHE_HPP_INCLUDED
