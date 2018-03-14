#ifndef LOCAL_COMMANDS_HPP_INCLUDED
#define LOCAL_COMMANDS_HPP_INCLUDED

#include <crapmud/script_util_shared.hpp>

#include <tinydir/tinydir.h>

#include "auto_handlers.hpp"

bool is_local_command(const std::string& command)
{
    if(command == "#")
        return true;

    if(starts_with(command, "#edit "))
        return true;

    if(starts_with(command, "#edit_es6 "))
        return true;

    if(starts_with(command, "#edit_es5 "))
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

std::string handle_local_command(const std::string& username, const std::string& command, auto_handler& auto_handle, bool& should_shutdown, terminal& term)
{
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

                if(names.size() >= 2)
                {
                    fname.push_back(names[1]);
                }
            }

            tinydir_next(&dir);
        }

        tinydir_close(&dir);

        std::string build;

        for(auto& i : fname)
            build += i + " ";

        return build;
    }

    if(starts_with(command, "#edit ") || starts_with(command, "#edit_es6 ") || starts_with(command, "#edit_es5 "))
    {
        std::vector<std::string> fname = no_ss_split(command, " ");

        if(fname.size() < 2)
            return make_error_col("Format is #edit scriptname");

        std::string name = fname[1];

        std::string es5_file_name = "scripts/" + username + "." + name + ".js";
        std::string es6_file_name = "scripts/" + username + "." + name + ".es6.js";

        std::string file_name = es5_file_name;

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

        std::string es5_file_name = "scripts/" + username + "." + name + ".js";
        std::string es6_file_name = "scripts/" + username + "." + name + ".es6.js";

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

    return "";
}

#endif // LOCAL_COMMANDS_HPP_INCLUDED
