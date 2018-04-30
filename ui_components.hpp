#ifndef UI_COMPONENTS_HPP_INCLUDED
#define UI_COMPONENTS_HPP_INCLUDED

#include <serialise/serialise.hpp>
#include "colour_interop.hpp"
#include "editable_string.hpp"
#include "auto_handlers.hpp"

struct vertical_scrollbar : serialisable
{
    float scrollbar_width = 5.f;
    float scroll_element_height = 5.f;

    ///so at 0 we're viewing the very bottom line, further up we're viewing higher lines
    int current_viewed_line_number = 0;

    void render(sf::RenderWindow& win, vec2f pos);
    virtual void do_serialise(serialise& s, bool ser);
};

#if 0
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

    void add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command = true);
};
#endif // 0


#endif // UI_COMPONENTS_HPP_INCLUDED
