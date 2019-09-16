#ifndef FORMAT_CACHE_HPP_INCLUDED
#define FORMAT_CACHE_HPP_INCLUDED

#include "colour_interop.hpp"
#include "string_helpers.hpp"

struct scrollbar_hack;

struct format_cache_2
{
    bool valid_cache = false;
    bool valid_last_line = false;
    bool was_focused = false;
    vec2f last_window_size = {0,0};

    vec2f last_content_size = {0,0};

    ///runs from top to bottom
    std::vector<std::vector<formatted_char>> line_cache;
    std::vector<int> height_cache;

    void ensure_built(vec2f window_dimensions, const std::vector<interop_vec_t>& all_chars);
    int get_lines() {int val = 0; for(auto& i : height_cache) val += i; return val;}

    void invalidate()
    {
        valid_cache = false;
    }

    void invalidate_last_line()
    {
        valid_last_line = false;
    }

    bool valid()
    {
        return valid_cache && valid_last_line;
    }

    void render_imgui(vec2f position, vec2f dim, float scroll_lines);
};

struct cacheable
{
    format_cache_2 cache;

    std::vector<std::string> raw_history;
    std::vector<interop_vec_t> history;
};

#endif // FORMAT_CACHE_HPP_INCLUDED
