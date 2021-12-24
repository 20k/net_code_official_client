#ifndef SCRIPT_TRANSFER_HPP_INCLUDED
#define SCRIPT_TRANSFER_HPP_INCLUDED

#include <string>
#include <nlohmann/json.hpp>

struct terminal2;

std::string default_up_handling(std::string_view user, std::string_view server_msg, const std::string& scripts_dir);

void filter_down_handling(terminal2& main_terminal, nlohmann::json& in);

#endif // SCRIPT_TRANSFER_HPP_INCLUDED
