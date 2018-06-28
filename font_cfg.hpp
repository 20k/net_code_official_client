#ifndef FONT_CFG_HPP_INCLUDED
#define FONT_CFG_HPP_INCLUDED

struct ImFont;

struct font_selector
{
    bool wants_rebuild = true;
    float fonts_multiply = 1.f;

    ///reset in constructor
    //unsigned int fonts_flags = ImGuiFreeType::ForceAutoHint | ImGuiFreeType::MonoHinting;

    unsigned int fonts_flags = 0;

    font_selector();

    bool is_open = false;

    enum class font_cfg
    {
        BASE = 0,
        TEXT_EDITOR = 1,
        DEFAULT = 2,
    };

    ImFont* get_base_font();
    ImFont* get_editor_font();

    // Call _BEFORE_ NewFrame()
    bool update_rebuild();
    // Call to draw interface
    void render();
};

#endif // FONT_CFG_HPP_INCLUDED
