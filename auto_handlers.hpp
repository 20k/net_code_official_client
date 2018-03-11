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
};

struct auto_handler
{
    bool use_autocomplete = false;

    int internal_state = 0;

    std::set<std::string> found_unprocessed_autocompletes;
    std::map<std::string, std::vector<autocomplete_args>> found_args;

    ///returns autocomplete
    void auto_colour(std::vector<interop_char>& in, bool colour_special = false);

    void handle_autocompletes(std::vector<interop_char>& in, int& cursor_idx);

    void clear_internal_state();
};

#endif // AUTO_HANDLERS_HPP_INCLUDED
