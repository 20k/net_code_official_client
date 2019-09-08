#include "window_context.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <libncclient/nc_util.hpp>
#include <nlohmann/json.hpp>
#include <libncclient/nc_util.hpp>

window_context::window_context()
{
    load();

    if(!glfwInit())
        throw std::runtime_error("Could not init glfw");

    /*sf::ContextSettings sett;
    sett.antialiasingLevel = 1;
    sett.sRgbCapable = is_srgb;

    win.create(sf::VideoMode(width, height), "net_code", sf::Style::Default, sett);
    win.resetGLStates();*/

            // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    window = glfwCreateWindow(width, height, "net_code_", NULL, NULL);
    if (window == NULL)
        throw std::runtime_error("Nullptr window in glfw");

    glfwMakeContextCurrent(window);

    if(glewInit() != GLEW_OK)
        throw std::runtime_error("Bad Glew");
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

        atomic_write_all(general_file, j.dump());
    }
    catch(...){}
}

void window_context::set_is_srgb(bool pis_srgb)
{
    is_srgb = pis_srgb;

    /*sf::ContextSettings sett;
    sett.antialiasingLevel = 1;
    sett.sRgbCapable = is_srgb;

    win.create(sf::VideoMode(width, height), "net_code", sf::Style::Default, sett);
    win.resetGLStates();*/
}
