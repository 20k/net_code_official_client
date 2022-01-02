#ifndef AUTO_HANDLERS_HPP_INCLUDED
#define AUTO_HANDLERS_HPP_INCLUDED

#include <vector>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <networking/serialisable.hpp>
#include <toolkit/clock.hpp>

struct render_string;
struct token_info;
struct connection_send_data;

struct autocomplete_args : serialisable
{
    std::string key;
    std::string arg;

    autocomplete_args(){}

    autocomplete_args(const std::string& key_, const std::string& arg_) : key(key_), arg(arg_) {}

    SERIALISE_SIGNATURE(autocomplete_args)
    {
        DO_SERIALISE(key);
        DO_SERIALISE(arg);
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
    steady_timer time_since_last_request;
    bool tab_pressed = false;

    bool use_autocomplete = false;
    bool use_autocolour = false;

    std::vector<std::string> found_unprocessed_autocompletes;
    std::map<std::string, std::vector<autocomplete_args>> found_args;
    std::map<std::string, int> is_valid;

    void handle_autocompletes(std::string& in, int& cursor_idx, int& cursor_offset, std::string& command_str);

    void extract_server_commands(nlohmann::json& in);
    void make_server_request(connection_send_data& send);

    SERIALISE_SIGNATURE(auto_handler)
    {
        DO_SERIALISE(found_unprocessed_autocompletes);
        DO_SERIALISE(found_args);
        DO_SERIALISE(is_valid);
    }
};

std::vector<std::string> parse_for_autocompletes(std::string_view view);
std::vector<render_string> auto_colour(std::string_view view, bool colour_like_terminal);

#endif // AUTO_HANDLERS_HPP_INCLUDED
