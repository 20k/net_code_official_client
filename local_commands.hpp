#ifndef LOCAL_COMMANDS_HPP_INCLUDED
#define LOCAL_COMMANDS_HPP_INCLUDED

#include <string>
#include <vector>

struct auto_handler;
struct terminal_imgui;

std::vector<std::string> get_scripts_list(const std::string& username);
std::string format_raw_script_name(const std::string& file_name);

std::string handle_local_command(const std::string& username, const std::string& command, auto_handler& auto_handle, bool& should_shutdown, terminal_imgui& term);

#endif // LOCAL_COMMANDS_HPP_INCLUDED
