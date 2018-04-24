#ifndef AUTO_HANDLERS_HPP_INCLUDED
#define AUTO_HANDLERS_HPP_INCLUDED

#include <vector>
#include <map>
#include <set>
#include <string>
#include <serialise/serialise.hpp>

struct interop_char;

struct autocomplete_args : serialisable
{
    std::string key;
    std::string arg;

    autocomplete_args(){}

    autocomplete_args(const std::string& key_, const std::string& arg_) : key(key_), arg(arg_) {}

    virtual void do_serialise(serialise& s, bool ser)
    {
        s.handle_serialise(key, ser);
        s.handle_serialise(arg, ser);
    }
};

struct specials_status
{
    bool has_open_paren = false;
    bool has_open_curly = false;
    bool has_close_curly = false;
    bool has_close_paren = false;
    bool has_trailing_comma = false;
};

struct auto_handler : serialisable
{
    bool tab_pressed = false;

    bool use_autocomplete = false;
    bool use_autocolour = false;

    bool window_in_focus = false;

    std::set<std::string> found_unprocessed_autocompletes;
    std::map<std::string, std::vector<autocomplete_args>> found_args;
    std::map<std::string, bool> is_valid;

    ///returns autocomplete
    void auto_colour(std::vector<interop_char>& in, bool colour_special = false, bool parse_for_autocompletes = true);

    void handle_autocompletes(std::vector<interop_char>& in, int& cursor_idx, int& cursor_offset, std::string& command_str);

    /*bool handle_script_autocomplete(std::vector<interop_char>& in, int& cursor_idx, std::string& command_str, const std::string& name);

    void handle_tab(const std::vector<interop_char>& in, int& cursor_idx, int parse_start,
                    const std::vector<autocomplete_args>& found, const specials_status& specials, std::string& command_str,
                    const std::vector<std::string>& to_skip);*/

    virtual void do_serialise(serialise& s, bool ser) override;
};

#endif // AUTO_HANDLERS_HPP_INCLUDED
