#include "local_commands.hpp"


#include <libncclient/nc_util.hpp>
#include <libncclient/c_shared_data.h>

#include <tinydir/tinydir.h>

#include "auto_handlers.hpp"
#include "imgui_ui_components.hpp"

#include <windows.h>
#include <shellapi.h>

std::string get_scripts_directory()
{
    return "./scripts";
}

std::vector<std::string> get_all_scripts_list()
{
    std::vector<std::string> ret;

    tinydir_dir dir;
    tinydir_open(&dir, "./scripts");

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if(!file.is_dir)
        {
            std::string name = file.name;

            if(no_ss_split(name, ".").size() >= 2)
                ret.push_back(name);
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);

    return ret;
}

std::vector<std::string> get_scripts_list(const std::string& username)
{
    std::vector<std::string> ret;

    tinydir_dir dir;
    tinydir_open(&dir, "./scripts");

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if(username != "")
        {
            if(starts_with(std::string(file.name), username + "."))
            {
                ret.push_back(file.name);
            }
        }
        else
        {
            if(!file.is_dir)
            {
                ret.push_back(file.name);
            }
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);

    return ret;
}

std::string format_raw_script_name(const std::string& file_name)
{
    auto names = no_ss_split(file_name, ".");

    if(names.size() == 2)
    {
        return names[1];
    }

    if(names.size() > 2)
    {
        if(names[2].size() > 0 && names[2] != "js")
            return names[1] + " [" + names[2] + "]";
        else
            return names[1];
    }

    return file_name;
}

std::string handle_local_command(const std::string& username, const std::string& command, auto_handler& auto_handle, bool& should_shutdown, terminal_imgui& term, chat_window& chat)
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
        clear_everything(term, chat);

    if(starts_with(command, "#clear_term"))
        term.clear_terminal();

    if(starts_with(command, "#clear_chat"))
        chat.clear_chat();

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

        ///removes tailing ", "
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
