#include "editable_string.hpp"

void editable_string::add_to_command(char c)
{
    if(cursor_pos_idx >= (int)command.size())
    {
        command.push_back(c);
    }
    else
    {
        command.insert(command.begin() + cursor_pos_idx, c);
    }

    cursor_pos_idx++;
}

void editable_string::remove_back()
{
    if(command.size() > 0)
        command.pop_back();
}

void editable_string::move_cursor(int dir)
{
    cursor_pos_idx += dir;
    cursor_pos_idx = clamp(cursor_pos_idx, 0, (int)command.size());
}

void editable_string::process_backspace()
{
    int to_remove = cursor_pos_idx - 1;

    if(to_remove < 0 || to_remove >= (int)command.size())
        return;

    command.erase(command.begin() + to_remove);

    cursor_pos_idx--;
}

void editable_string::process_delete()
{
    int to_remove = cursor_pos_idx;

    if(to_remove < 0 || to_remove >= (int)command.size())
        return;

    command.erase(command.begin() + to_remove);
}


void editable_string::move_command_history_idx(int dir)
{
    command_history_idx += dir;

    command_history_idx = clamp(command_history_idx, 0, (int)command_history.size());

    if(command_history_idx >= 0 && command_history_idx < (int)command_history.size())
    {
        command = command_history[command_history_idx];
    }

    if(command_history_idx == (int)command_history.size())
    {
        ///ideally we'd reset to partially held commands
        command = "";
    }

    cursor_pos_idx = command.size();
}

void editable_string::clear_command()
{
    command = "";
    command_history_idx = command_history.size();
    cursor_pos_idx = 0;
}

void editable_string::push_command_to_history(const std::string& cmd)
{
    int max_command_history = 1000;

    limit_size(command_history, max_command_history);

    if(command_history.size() > 0)
    {
        int fin = (int)command_history.size() - 1;

        if(command_history[fin] == cmd)
            return;
    }

    command_history.push_back(cmd);
    command_history_idx = (int)command_history.size();
}
