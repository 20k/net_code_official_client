#ifndef FORMAT_CACHE_HPP_INCLUDED
#define FORMAT_CACHE_HPP_INCLUDED

#include "colour_interop.hpp"
#include "string_helpers.hpp"

struct scrollbar_hack;

struct format_cache
{
    vec2f last_pos = {0,0};

    int total_lines = 0;
    float scrolled_cache = 0;
    bool valid_cache = false;
    bool last_line_valid = false;
    std::vector<std::vector<formatted_char>> out;

    std::map<int, int> height_map_cache;

    std::map<int, bool> initialised_cache;
    std::map<int, std::vector<formatted_char>> line_cache;
    std::vector<interop_vec_t> interop_cache;

    bool deferred_invalidate = false;

    vec2f cached_start;
    vec2f cached_dim;
    vec2f cached_window_size;
    float cached_line_offset = 0;
    float cached_y_end = 0;

    bool was_focused = false;

    void last_line_invalidate()
    {
        last_line_valid = false;
    }

    void invalidate()
    {
        valid_cache = false;
        last_line_valid = false;
    }

    void ensure_last_line(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns);
    void ensure_built(vec2f current, vec2f start, vec2f wrap_dim, const std::vector<interop_vec_t>& all_interop, scrollbar_hack& scroll_hack, int vertical_columns);

    std::vector<formatted_char> request(int render_y_start, int render_y_end);

    std::vector<std::vector<formatted_char>> get_render_cache();


    int get_lines()
    {
        return total_lines;
    }

    bool valid()
    {
        return valid_cache && last_line_valid;
    }
};

struct cacheable
{
    format_cache cache;

    std::vector<std::string> raw_history;
    std::vector<interop_vec_t> history;
};

#endif // FORMAT_CACHE_HPP_INCLUDED
