#ifndef AUTO_HANDLERS_HPP_INCLUDED
#define AUTO_HANDLERS_HPP_INCLUDED

#include <vector>
#include <map>
#include <set>
#include <string>

struct interop_char;

struct autocomplete_args
{
    std::string key;
    std::string arg;
    bool valid = true;
};

struct specials_status
{
    bool has_open_paren = false;
    bool has_open_curly = false;
    bool has_close_curly = false;
    bool has_close_paren = false;
    bool has_trailing_comma = false;
};

struct auto_handler
{
    bool use_autocomplete = false;
    bool use_autocolour = false;

    bool window_in_focus = false;

    int internal_state = 0;

    std::set<std::string> found_unprocessed_autocompletes;
    std::map<std::string, std::vector<autocomplete_args>> found_args;

    ///returns autocomplete
    void auto_colour(std::vector<interop_char>& in, bool colour_special = false);

    void handle_autocompletes(std::vector<interop_char>& in, int& cursor_idx, std::string& command_str);

    void handle_tab(const std::vector<interop_char>& in, int& cursor_idx, int parse_start,
                    const std::vector<autocomplete_args>& found, const specials_status& specials, std::string& command_str,
                    const std::vector<std::string>& to_skip);

    void clear_internal_state();
};

#endif // AUTO_HANDLERS_HPP_INCLUDED
