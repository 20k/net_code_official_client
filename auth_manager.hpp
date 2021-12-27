#ifndef AUTH_MANAGER_HPP_INCLUDED
#define AUTH_MANAGER_HPP_INCLUDED

#include <string>
#include <nlohmann/json.hpp>

struct connection_send_data;
struct steamapi;
struct terminal2;

struct auth_manager
{
    bool am_authenticated = false;

    void check(steamapi& s_api, connection_send_data& to_write, const std::string& current_user);
    void display(terminal2& term, steamapi& s_api, connection_send_data& to_write, const std::string& current_user);

    void extract_server_commands(terminal2& output, nlohmann::json& in);

private:
    bool should_display_dialogue = false;
    std::string auth_dialogue_text;
};

#endif // AUTH_MANAGER_HPP_INCLUDED
