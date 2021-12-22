#ifndef EDITABLE_STRING_HPP_INCLUDED
#define EDITABLE_STRING_HPP_INCLUDED

#include <networking/serialisable.hpp>

struct editable_string : serialisable
{
    int cursor_pos_idx = 0;
    std::string command;
    std::vector<std::string> command_history;
    int command_history_idx = 0;

    SERIALISE_SIGNATURE(editable_string)
    {
        DO_SERIALISE(cursor_pos_idx);
        DO_SERIALISE(command);
        DO_SERIALISE(command_history);
        DO_SERIALISE(command_history_idx);
    }

    void add_to_command(char c);
    void remove_back();
    void move_cursor(int dir);
    void process_backspace();
    void process_delete();
    void move_command_history_idx(int dir);

    void clear_command();
    void push_command_to_history(std::string_view cmd);
};

template<typename T>
void limit_size(T& t, int max_size)
{
    while((int)t.size() >= max_size)
    {
        t.erase(t.begin());
    }
}

#endif // EDITABLE_STRING_HPP_INCLUDED
