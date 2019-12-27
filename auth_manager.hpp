#ifndef AUTH_MANAGER_HPP_INCLUDED
#define AUTH_MANAGER_HPP_INCLUDED

#include <string>

struct connection;
struct steamapi;
struct terminal_imgui;

struct auth_manager
{
    bool am_authenticated = false;

    void check(steamapi& s_api, connection& conn, const std::string& current_user);
    void display(terminal_imgui& term, steamapi& s_api, connection& conn, const std::string& current_user);

private:
    bool should_display_dialogue = false;
    std::string auth_dialogue_text;
};

#endif // AUTH_MANAGER_HPP_INCLUDED
