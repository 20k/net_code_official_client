#include "window_context.hpp"
#include <libncclient/nc_util.hpp>
#include <json/json.hpp>

window_context::window_context()
{
    load();

    sf::ContextSettings sett;
    sett.antialiasingLevel = 1;
    sett.sRgbCapable = is_srgb;

    win.create(sf::VideoMode(width, height), "net_code", sf::Style::Default, sett);
    win.resetGLStates();
}

void window_context::load()
{
    std::string general_file = "./window.txt";

    if(file_exists(general_file))
    {
        using nlohmann::json;

        std::string str = read_file_bin(general_file);

        try
        {
            json j;
            j = json::parse(str);

            if(j.find("width_px") != j.end() && j.find("height_px") != j.end())
            {
                width = j["width_px"];
                height = j["height_px"];
                //window.setSize(sf::Vector2u((int)j["width_px"], (int)j["height_px"]));
            }

            if(j.find("is_srgb") != j.end())
            {
                is_srgb = j["is_srgb"];
            }
        }
        catch(...){}
    }
}

void window_context::save()
{
    std::string general_file = "./window.txt";

    try
    {
        using nlohmann::json;

        json j;
        j["width_px"] = width;
        j["height_px"] = height;
        j["is_srgb"] = is_srgb;

        write_all_bin(general_file, j.dump());
    }
    catch(...){}
}

void window_context::set_is_srgb(bool pis_srgb)
{
    is_srgb = pis_srgb;

    sf::ContextSettings sett;
    sett.antialiasingLevel = 1;
    sett.sRgbCapable = is_srgb;

    win.create(sf::VideoMode(width, height), "net_code", sf::Style::Default, sett);
    win.resetGLStates();
}
