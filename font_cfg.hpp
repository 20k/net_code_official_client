#ifndef FONT_CFG_HPP_INCLUDED
#define FONT_CFG_HPP_INCLUDED

#include <networking/serialisable.hpp>
#include <SFML/Graphics.hpp>

struct ImFont;

struct window_context;

struct font_selector : serialisable
{
    bool wants_rebuild = true;
    float fonts_multiply = 1.f;

    ///reset in constructor
    //unsigned int fonts_flags = ImGuiFreeType::ForceAutoHint | ImGuiFreeType::MonoHinting;

    unsigned int fonts_flags = 0;
    unsigned int subpixel_flags = 1;

    font_selector();

    bool is_open = false;

    enum class font_cfg
    {
        BASE = 0,
        TEXT_EDITOR = 1,
        DEFAULT = 2,
    };

    int current_base_font = (int)font_cfg::BASE;

    ImFont* get_base_font();
    ImFont* get_editor_font();

    float current_base_font_size = 14;

    void reset_default_fonts(float editor_font_size = 14.f);
    // Call _BEFORE_ NewFrame()
    bool update_rebuild(sf::RenderWindow& win, float editor_font_size = 14.f);
    // Call to draw interface
    void render(window_context& window_ctx);

    SERIALISE_SIGNATURE(font_selector)
    {
        DO_SERIALISE(current_base_font_size);
        DO_SERIALISE(current_base_font);
    }

    //sf::Texture font_atlas;
};

struct window_context;

struct font_render_context
{
    font_selector& font_select;
    window_context& window_ctx;

    font_render_context(font_selector& pfont_select, window_context& pwindow_ctx) : font_select(pfont_select), window_ctx(pwindow_ctx)
    {

    }
};

#endif // FONT_CFG_HPP_INCLUDED
