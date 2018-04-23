#ifndef LOCAL_COMMANDS_HPP_INCLUDED
#define LOCAL_COMMANDS_HPP_INCLUDED

#include <libncclient/nc_util.hpp>
#include <libncclient/c_shared_data.h>

#include <tinydir/tinydir.h>

#include "auto_handlers.hpp"

std::string handle_local_command(const std::string& username, const std::string& command, auto_handler& auto_handle, bool& should_shutdown, terminal& term)
{
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
    {
        term.clear_text();
    }

    if(starts_with(command, "#clear_term"))
        term.clear_terminal();

    if(starts_with(command, "#clear_chat"))
        term.clear_chat();

    if(username == "")
        return "Please log in with user <username>";

    if(command == "#")
    {
        std::vector<std::string> fname;

        tinydir_dir dir;
        tinydir_open(&dir, "./scripts");

        while (dir.has_next)
        {
            tinydir_file file;
            tinydir_readfile(&dir, &file);

            if(starts_with(std::string(file.name), username + "."))
            {
                auto names = no_ss_split(file.name, ".");

                if(names.size() == 2)
                {
                    fname.push_back(names[1]);
                }
                if(names.size() > 2)
                {
                    if(names[2].size() > 0 && names[2] != "js")
                        fname.push_back(names[1] + " [" + names[2] + "]");
                    else
                        fname.push_back(names[1]);
                }
            }

            tinydir_next(&dir);
        }

        tinydir_close(&dir);

        std::string build;

        for(auto& i : fname)
            build += i + ", ";

        if(build.size() > 0)
        {
            build.pop_back();
            build.pop_back();
        }

        return build;
    }

    if(starts_with(command, "#edit ") || starts_with(command, "#edit_es6 ") || starts_with(command, "#edit_es5 "))
    {
        std::vector<std::string> fname = no_ss_split(command, " ");

        if(fname.size() < 2)
            return make_error_col("Format is #edit scriptname");

        std::string name = fname[1];

        std::string es5_file_name = "scripts/" + username + "." + name + ".es5.js";
        std::string es6_file_name = "scripts/" + username + "." + name + ".js";

        std::string file_name = es6_file_name;

        ///#edit defaults to opening an es6 file
        ///but defaults to creating an es5 file
        if(file_exists(es6_file_name) && !file_exists(file_name))
            file_name = es6_file_name;

        bool requested_es6 = starts_with(command, "#edit_es6 ");
        bool requested_es5 = starts_with(command, "#edit_es5 ");

        if(requested_es6)
        {
            file_name = es6_file_name;

            if(file_exists(es5_file_name) && !file_exists(es6_file_name))
            {
                rename(es5_file_name.c_str(), es6_file_name.c_str());
            }
        }

        if(requested_es5)
        {
            file_name = es5_file_name;

            if(file_exists(es6_file_name) && !file_exists(es5_file_name))
            {
                rename(es6_file_name.c_str(), es5_file_name.c_str());
            }
        }

        if(!file_exists(file_name))
        {
            write_all_bin(file_name, "function(context, args)\n{\n\n}");
        }

        ///need to use shellexecute to prevent hangs on no js
        system(("start " + file_name).c_str());
    }

    if(starts_with(command, "#open "))
    {
        std::vector<std::string> fname = no_ss_split(command, " ");

        if(fname.size() < 2)
            return make_error_col("Format is #open scriptname");

        std::string name = fname[1];

        std::string es5_file_name = "scripts/" + username + "." + name + ".es5.js";
        std::string es6_file_name = "scripts/" + username + "." + name + ".js";

        std::string file_name = es5_file_name;

        if(file_exists(es6_file_name))
            file_name = es6_file_name;

        if(!file_exists(file_name))
            return "No such file";

        system(("start " + file_name).c_str());
    }

    if(starts_with(command, "#dir"))
    {
        ShellExecute(NULL, "open", "scripts", NULL, NULL, SW_SHOWDEFAULT);
    }

    return "";
}

inline
std::string handle_up(c_shared_data shared, const std::string& unknown_command)
{
    std::string up = "client_command #up ";
    std::string up_es6 = "client_command #up_es6 ";
    std::string dry = "client_command #dry ";

    std::vector<std::string> strings = no_ss_split(unknown_command, " ");

    if((starts_with(unknown_command, up) || starts_with(unknown_command, dry)) && strings.size() == 3)
    {
        std::string name = strings[2];

        char* c_user = sd_get_user(shared);
        std::string hardcoded_user(c_user);
        free_string(c_user);

        std::string diskname = "./scripts/" + hardcoded_user + "." + name + ".es5.js";
        std::string diskname_es6 = "./scripts/" + hardcoded_user + "." + name + ".js";

        std::string comm = up;

        if(starts_with(unknown_command, dry))
            comm = dry;

        std::string data = "";

        if(file_exists(diskname))
            data = read_file(diskname);

        if(file_exists(diskname_es6))
        {
            data = read_file(diskname_es6);
            comm = up_es6;
        }

        std::string final_command = comm + name + " " + data;

        return final_command;
    }

    return unknown_command;
}

#endif // LOCAL_COMMANDS_HPP_INCLUDED
