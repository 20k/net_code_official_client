#ifndef FORMAT_CACHE_HPP_INCLUDED
#define FORMAT_CACHE_HPP_INCLUDED

#include "colour_interop.hpp"
#include "string_helpers.hpp"

struct scrollbar_hack;

struct format_cache
{
    vec2f last_pos = {0,0};

    int total_lines = 0;
    bool valid_cache = false;
    std::vector<std::vector<formatted_char>> full_cache;
    std::vector<std::vector<formatted_char>> render_cache;
    std::vector<std::vector<formatted_char>> out;

    std::map<int, int> y_internal_pos_to_index;

    vec2f cached_start;
    vec2f cached_dim;
    float cached_line_offset = 0;
    float cached_y_end = 0;

    void invalidate()
    {
        valid_cache = false;
    }

    void ensure_built(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns);

    std::vector<std::vector<formatted_char>> get_render_cache();

    std::vector<std::vector<formatted_char>> get_cache()
    {
        return render_cache;
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
