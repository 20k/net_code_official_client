#include "local_commands.hpp"

#include <toolkit/fs_helpers.hpp>

#include <tinydir/tinydir.h>
#include <libncclient/nc_util.hpp>

#include "auto_handlers.hpp"

#include <iostream>
#include <string_view>

std::string get_scripts_directory(std::string_view username)
{
    #ifndef __EMSCRIPTEN__
    #ifdef __WIN32__
    return "scripts\\" + std::string(username.begin(), username.end());
    #else
    return "scripts/" + std::string(username.begin(), username.end());
    #endif // __WIN32__
    #else
    return "scripts";
    #endif
}

void ipc_open(const std::string& fname)
{
    system(("start " + fname).c_str());
}

void open_file(std::string_view name)
{
    std::string sname(name.begin(), name.end());

    system(("start " + sname).c_str());
}

void open_directory(std::string_view name)
{
    std::string sname(name.begin(), name.end());

    system(("start " + sname).c_str());
}

std::string get_scripts_list(std::string_view username)
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

bool is_local_command(std::string_view command)
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
