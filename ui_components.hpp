#ifndef UI_COMPONENTS_HPP_INCLUDED
#define UI_COMPONENTS_HPP_INCLUDED

#include <serialise/serialise.hpp>
#include "colour_interop.hpp"
#include "editable_string.hpp"
#include "auto_handlers.hpp"

struct chat_thread : serialisable
{
    std::vector<interop_vec_t> chats;

    virtual void do_serialise(serialise& s, bool ser);
};

struct button : serialisable
{
    std::string txt;
    bool is_selected = false;

    vec2f pos;
    vec2f dim;

    button();
    button(const std::string& txt, bool is_selected = false);

    bool within(vec2f mpos);
    virtual void do_serialise(serialise& s, bool ser);
};

struct chat_window : serialisable
{
    vec2f render_start = {0,0};
    vec2i dim = {500, 300};
    vec3f frame_col = {0.46f, 0.8f, 1.f};

    vec2f side_dim = {100, dim.y()};

    std::vector<button> side_buttons
    {
        {"0000"},
        {"7001"},
        {"memes"}
    };

    std::string selected = "0000";
    editable_string command;

    auto_handler auto_handle;

    virtual void do_serialise(serialise& s, bool ser);

    sf::Color get_frame_col();
    sf::Color get_highlight_col();

    bool focused = false;
    float border_size = 2.f;

    void tick();

    void render(sf::RenderWindow& win, std::map<std::string, chat_thread>& threads);

    void render_side_attachment(sf::RenderWindow& win);

    void process_click(vec2f pos);

    bool within(vec2f pos);

    void set_side_channels(const std::vector<std::string>& sides);
};

struct terminal : serialisable
{
    std::vector<interop_vec_t> text_history;

    std::map<std::string, chat_thread> chat_threads;

    sf::Font font;

    bool focused = true;
    editable_string command;

    auto_handler auto_handle;

    virtual void do_serialise(serialise& s, bool ser);

    void clear_terminal();
    void clear_chat();
    void clear_text();

    terminal();
    void render(sf::RenderWindow& win);
    void bump_command_to_history();

    void get_chat_api_strs(const std::string& chat_in, std::vector<std::string>& channels, std::vector<std::string>& msgs, std::vector<std::string>& in_channels);

    void parse_scriptargs(const std::string& in);
    void add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command = true);
};


#endif // UI_COMPONENTS_HPP_INCLUDED
