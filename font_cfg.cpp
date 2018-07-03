#include "font_cfg.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui-sfml/imgui-SFML.h>
#include <imgui/misc/freetype/imgui_freetype.h>

#include <SFML/Graphics.hpp>

#include "string_helpers.hpp"

void update_font_texture_safe()
{
    static sf::Texture texture;

    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    texture.create(width, height);
    texture.update(pixels);

    io.Fonts->TexID = reinterpret_cast<void*>(texture.getNativeHandle());
}

font_selector::font_selector()
{
    fonts_flags = ImGuiFreeType::ForceAutoHint;
    //fonts_flags = ImGuiFreeType::ForceAutoHint | ImGuiFreeType::MonoHinting;
}

ImFont* font_selector::get_base_font()
{
    ImGuiIO& io = ImGui::GetIO();

    return io.Fonts->Fonts[(int)font_cfg::BASE];
}

ImFont* font_selector::get_editor_font()
{
    ImGuiIO& io = ImGui::GetIO();

    return io.Fonts->Fonts[(int)font_cfg::TEXT_EDITOR];
}

void font_selector::reset_default_fonts(float editor_font_size)
{
    ImFontConfig font_cfg;
    font_cfg.GlyphExtraSpacing = ImVec2(char_inf::extra_glyph_spacing, 0);

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->Clear();
    //io.Fonts->ClearFonts();

    ///BASE
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", 14.f, &font_cfg);
    ///TEXT_EDITOR
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", editor_font_size, &font_cfg);
    ///DEFAULT
    io.Fonts->AddFontDefault();

    //ImGui::PushFont(font);

    wants_rebuild = true;
}

// Call _BEFORE_ NewFrame()
bool font_selector::update_rebuild()
{
    if (!wants_rebuild)
        return false;

    ImGuiIO& io = ImGui::GetIO();

    for (int n = 0; n < io.Fonts->Fonts.Size; n++)
    {
        //io.Fonts->Fonts[n]->ConfigData->RasterizerMultiply = FontsMultiply;
        //io.Fonts->Fonts[n]->ConfigData->RasterizerFlags = (BuildMode == FontBuildMode_FreeType) ? fonts_flags : 0x00;
    }

    ImGuiFreeType::BuildFontAtlas(io.Fonts, fonts_flags);

    wants_rebuild = false;
    update_font_texture_safe();

    //ImGui::PushFont(io.Fonts->Fonts[0]);
    return true;
}

// Call to draw interface
void font_selector::render()
{
    if(!is_open)
        return;

    ImGui::Begin("FreeType Options", &is_open);
    ImGui::ShowFontSelector("Fonts");

    wants_rebuild |= ImGui::DragFloat("Multiply", &fonts_multiply, 0.001f, 0.0f, 2.0f);

    wants_rebuild |= ImGui::CheckboxFlags("NoHinting",     &fonts_flags, ImGuiFreeType::NoHinting);
    wants_rebuild |= ImGui::CheckboxFlags("NoAutoHint",    &fonts_flags, ImGuiFreeType::NoAutoHint);
    wants_rebuild |= ImGui::CheckboxFlags("ForceAutoHint", &fonts_flags, ImGuiFreeType::ForceAutoHint);
    wants_rebuild |= ImGui::CheckboxFlags("LightHinting",  &fonts_flags, ImGuiFreeType::LightHinting);
    wants_rebuild |= ImGui::CheckboxFlags("MonoHinting",   &fonts_flags, ImGuiFreeType::MonoHinting);
    wants_rebuild |= ImGui::CheckboxFlags("Bold",          &fonts_flags, ImGuiFreeType::Bold);
    wants_rebuild |= ImGui::CheckboxFlags("Oblique",       &fonts_flags, ImGuiFreeType::Oblique);

    ImGui::End();
}
