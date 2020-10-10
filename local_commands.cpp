#include "local_commands.hpp"

#include <toolkit/fs_helpers.hpp>

#include <tinydir/tinydir.h>
#include <libncclient/nc_util.hpp>

#include "auto_handlers.hpp"
#include "imgui_ui_components.hpp"

#include <iostream>
#include <string_view>

std::string get_scripts_directory(const std::string& username)
{
    #ifndef __EMSCRIPTEN__
    #ifdef __WIN32__
    return "scripts\\" + username;
    #else
    return "scripts/" + username;
    #endif // __WIN32__
    #else
    return "scripts";
    #endif
}

void ipc_open(const std::string& fname)
{
    system(("start " + fname).c_str());
}

std::string handle_local_command(const std::string& username, const std::string& command, auto_handler& auto_handle, bool& should_shutdown, terminal_manager& terminals, chat_window& chat)
{
    #ifndef __EMSCRIPTEN__
    file::mkdir("scripts");
    file::mkdir(get_scripts_directory(username).c_str());
    #endif // __EMSCRIPTEN__

    if(starts_with(command, "#clear_autos") || starts_with(command, "#autos_clear"))
    {
        auto_handle.found_args.clear();
        auto_handle.is_valid.clear();
    }

    if(starts_with(command, "#shutdown"))
    {
        should_shutdown = true;
    }

    if(starts_with(command, "#cls"))
        clear_everything(terminals, chat);

    if(starts_with(command, "#clear_term"))
        terminals.get_focused_terminal()->clear_terminal();

    if(starts_with(command, "#clear_chat"))
        chat.clear_chat();

    if(username == "")
        return "Please log in with user <username>";

    if(command == "#")
    {
        std::vector<std::string> fname;

        tinydir_dir dir;
        #ifndef __EMSCRIPTEN__
        tinydir_open(&dir, get_scripts_directory(username).c_str());
        #else
        tinydir_open(&dir, ("web/" + get_scripts_directory(username)).c_str());
        #endif

        while (dir.has_next)
        {
            tinydir_file file;
            tinydir_readfile(&dir, &file);

            if(!file.is_dir)
            {
                auto post_split = no_ss_split(file.name, ".");

                if(post_split.size() == 2 && post_split[1] == "js")
                    fname.push_back(post_split[0]);
            }

            tinydir_next(&dir);
        }

        tinydir_close(&dir);

        std::string build;

        for(auto& i : fname)
            build += i + ", ";

        ///removes tailing ", "
        if(build.size() > 0)
        {
            build.pop_back();
            build.pop_back();
        }

        return build;
    }

    if(starts_with(command, "#edit "))
    {
        std::vector<std::string> fname = no_ss_split(command, " ");

        if(fname.size() < 2)
            return make_error_col("Format is #edit scriptname");

        std::string name = fname[1];

        std::string file_name = get_scripts_directory(username) + "/" + name + ".js";

        if(!file::exists(file_name))
        {
            std::cout << "Trying to create " << file_name << std::endl;

            file::write(file_name, "function(context, args)\n{\n\n}", file::mode::TEXT);
        }

        ipc_open(file_name);
    }

    if(starts_with(command, "#open "))
    {
        std::vector<std::string> fname = no_ss_split(command, " ");

        if(fname.size() < 2)
            return make_error_col("Format is #open scriptname");

        std::string name = fname[1];

        std::string file_name = get_scripts_directory(username) + "/" + name + ".js";

        if(!file::exists(file_name))
            return "No such file";

        ipc_open(file_name);
    }

    if(starts_with(command, "#dir"))
    {
        system(("start " + get_scripts_directory(username)).c_str());
        //ShellExecute(NULL, "open", "scripts", NULL, NULL, SW_SHOWDEFAULT);
    }

    return "";
}

bool is_local_command(const std::string& command)
{
    if(command == "#")
        return true;

    if(starts_with(command, "#edit "))
        return true;

    if(starts_with(command, "#open "))
        return true;

    if(starts_with(command, "#dir"))
        return true;

    if(starts_with(command, "#clear_autos"))
        return true;

    if(starts_with(command, "#autos_clear"))
        return true;

    if(starts_with(command, "#shutdown"))
        return true;

    if(starts_with(command, "#cls"))
        return true;

    if(starts_with(command, "#clear_term"))
        return true;

    if(starts_with(command, "#clear_chat"))
        return true;

    return false;
}
