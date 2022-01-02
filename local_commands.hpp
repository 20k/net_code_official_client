#ifndef LOCAL_COMMANDS_HPP_INCLUDED
#define LOCAL_COMMANDS_HPP_INCLUDED

#include <string>
#include <string_view>
#include <vector>

struct auto_handler;
struct terminal_imgui;
struct chat_window;
struct terminal_manager;

void open_file(std::string_view name);
void open_directory(std::string_view name);

std::string get_scripts_list(std::string_view username);
std::string get_scripts_directory(std::string_view username);

bool is_local_command(std::string_view str);

#endif // LOCAL_COMMANDS_HPP_INCLUDED
