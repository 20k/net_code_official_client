#ifndef LOCAL_COMMANDS_HPP_INCLUDED
#define LOCAL_COMMANDS_HPP_INCLUDED

#include <crapmud/script_util_shared.hpp>

#include <tinydir/tinydir.h>

bool is_local_command(const std::string& command)
{
    if(command == "#")
        return true;

    if(starts_with(command, "#edit "))
        return true;

    if(starts_with(command, "#dir"))
        return true;

    return false;
}

std::string handle_local_command(const std::string& username, const std::string& command)
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

    if(starts_with(command, "#edit "))
    {
        std::vector<std::string> fname = no_ss_split(command, " ");

        if(fname.size() < 2)
            return make_error_col("Format is #edit scriptname");

        std::string name = fname[1];

        std::string file_name = "scripts/" + username + "." + name + ".js";

        if(!file_exists(file_name))
        {
            write_all_bin(file_name, "function(context, args)\n{\n\n}");
        }

        ///need to use shellexecute to prevent hangs on no js
        system(("start " + file_name).c_str());
    }

    if(starts_with(command, "#dir"))
    {
        ShellExecute(NULL, "open", "scripts", NULL, NULL, SW_SHOWDEFAULT);
    }

    return "";
}

#endif // LOCAL_COMMANDS_HPP_INCLUDED
