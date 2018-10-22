#ifndef FONT_CFG_HPP_INCLUDED
#define FONT_CFG_HPP_INCLUDED

#include <serialise/serialise.hpp>

struct ImFont;

sf::Texture& get_font_atlas();

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
    void render();

    virtual void do_serialise(serialise& s, bool ser) override
    {
        s.handle_serialise(current_base_font_size, ser);
        s.handle_serialise(current_base_font, ser);
    }

    std::vector<sf::Font> sfml_fonts;

    sf::Font* get_base_sfml_font();
};

struct font_render_context
{
    sf::RenderWindow& win;
    font_selector& font_select;

    font_render_context(sf::RenderWindow& pwin, font_selector& pfont_select) : win(pwin), font_select(pfont_select)
    {

    }
};

#endif // FONT_CFG_HPP_INCLUDED
