#ifndef UI_STACK_HPP_INCLUDED
#define UI_STACK_HPP_INCLUDED

#include <stdint.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct connection_send_data;
struct ui_stack;

struct ui_element
{
    std::string type;
    std::string element_id;
    std::vector<nlohmann::json> arguments;
    uint64_t authoritative_until_sequence_id = 0;

    bool was_hovered = false;
    bool was_active = false;
    bool was_focused = false;
    bool was_visible = false;
    bool was_return_true = false;

    bool last_treenode_state = false;
};

struct ui_stack
{
    std::vector<ui_element> elements;
    std::vector<std::string> typelist;
};

std::string get_element_id(const std::string& type, const std::vector<nlohmann::json>& data);
void render_ui_stack(connection_send_data& to_write, uint64_t& sequence_id_in, ui_stack& stk, int id, bool is_linear_colour);

#endif // UI_STACK_HPP_INCLUDED
