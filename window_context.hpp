#ifndef WINDOW_CONTEXT_HPP_INCLUDED
#define WINDOW_CONTEXT_HPP_INCLUDED

struct GLFWwindow;

struct window_context
{
    int width = 1400;
    int height = 600;

    bool is_srgb = true;

    window_context();

    void load();
    void save();

    void set_width_height(int w, int h);
    void set_is_srgb(bool is_srgb);

    GLFWwindow* window = nullptr;
    const char* glsl_version = nullptr;

    bool srgb_dirty = false;
};

#endif // WINDOW_CONTEXT_HPP_INCLUDED
