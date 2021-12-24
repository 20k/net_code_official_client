#ifndef LOCAL_COMMANDS_HPP_INCLUDED
#define LOCAL_COMMANDS_HPP_INCLUDED

#include <string>
#include <string_view>
#include <vector>

struct auto_handler;
struct terminal_imgui;
struct chat_window;
struct terminal_manager;

std::string get_scripts_directory(const std::string& username);

std::string handle_local_command(const std::string& username, std::string_view command, auto_handler& auto_handle, bool& should_shutdown, terminal_manager& terminals, chat_window& chat);
bool is_local_command(std::string_view str);

#endif // LOCAL_COMMANDS_HPP_INCLUDED
