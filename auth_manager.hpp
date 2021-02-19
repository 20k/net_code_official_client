#ifndef AUTH_MANAGER_HPP_INCLUDED
#define AUTH_MANAGER_HPP_INCLUDED

#include <string>

struct connection_send_data;
struct steamapi;
struct terminal_manager;

struct auth_manager
{
    bool am_authenticated = false;

    void check(steamapi& s_api, connection_send_data& to_write, const std::string& current_user);
    void display(terminal_manager& term, steamapi& s_api, connection_send_data& to_write, const std::string& current_user);

private:
    bool should_display_dialogue = false;
    std::string auth_dialogue_text;
};

#endif // AUTH_MANAGER_HPP_INCLUDED
