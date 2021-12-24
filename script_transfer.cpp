#include "script_transfer.hpp"
#include <vector>
#include <libncclient/nc_util.hpp>
#include <toolkit/fs_helpers.hpp>
#include "render_string.hpp"
#include "local_commands.hpp"

std::string default_up_handling(std::string_view user, std::string_view server_msg, const std::string& scripts_dir)
{
    std::string up = "#up ";
    std::string dry = "#dry ";

    std::vector<std::string> strings = no_ss_split(std::string(server_msg.begin(), server_msg.end()), " ");

    if((starts_with(server_msg, up) || starts_with(server_msg, dry)) && strings.size() == 2)
    {
        std::string name = strings[1];

        std::string diskname = scripts_dir + name + ".js";

        std::string comm = up;

        if(starts_with(server_msg, dry))
            comm = dry;

        std::string data = "";

        if(file::exists(diskname))
            data = file::read(diskname, file::mode::TEXT);

        return comm + name + " " + data;
    }

    return std::string(server_msg.begin(), server_msg.end());
}

void filter_down_handling(terminal2& main_terminal, nlohmann::json& in)
{
    std::string type = in["type"];

    if(type == "script_down")
    {
        std::string name = in["name"];
        std::string data = in["data"];

        auto post_split = no_ss_split(name, ".");

        if(post_split.size() >= 2)
        {
            std::string save_name = get_scripts_directory(post_split[0]) + "\\" + post_split[1] + ".down.js";

            file::write(save_name, data, file::mode::TEXT);

            #ifdef __EMSCRIPTEN__
            std::string webdlname = name + ".down.js";
            file::download(webdlname, data);
            #endif // __EMSCRIPTEN__

            main_terminal.add_main_text(make_success_col("Downloaded and saved script to " + get_scripts_directory(post_split[0]) + "\\" + post_split[1] + ".down.js") + "\n");
        }
    }
}

