#include "font_cfg.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/freetype/imgui_freetype.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "string_helpers.hpp"
#include <tinydir/tinydir.h>
#include <iostream>
#include <toolkit/render_window.hpp>
#include <toolkit/fs_helpers.hpp>

font_selector::font_selector()
{
    fonts_flags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_LCD | ImGuiFreeTypeBuilderFlags_FILTER_DEFAULT | ImGuiFreeTypeBuilderFlags_LoadColor;
}

ImFont* font_selector::get_base_font()
{
    ImGuiIO& io = ImGui::GetIO();

    return io.Fonts->Fonts[current_base_font];
}

ImFont* font_selector::get_square_font()
{
    ImGuiIO& io = ImGui::GetIO();

    return io.Fonts->Fonts[1];
}

ImFont* font_selector::get_editor_font()
{
    ImGuiIO& io = ImGui::GetIO();

    return io.Fonts->Fonts[(int)font_cfg::TEXT_EDITOR];
}

void font_selector::find_saved_font()
{
    ImGuiIO& io = ImGui::GetIO();

    for(int n = 0; n < io.Fonts->Fonts.Size; n++)
    {
        std::string str = io.Fonts->Fonts[n]->GetDebugName();

        if(str == current_font_name)
        {
            wants_rebuild = true;
            current_base_font = n;
        }
    }
}

struct tinydir_autoclose
{
    tinydir_dir dir;

    tinydir_autoclose(const std::string& directory)
    {
        tinydir_open(&dir, directory.c_str());
    }

    ~tinydir_autoclose()
    {
        tinydir_close(&dir);
    }
};

template<typename T>
void for_each_file(const std::string& directory, const T& t)
{
    tinydir_autoclose close(directory);

    while(close.dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&close.dir, &file);

        if(!file.is_dir)
        {
            std::string file_name(file.name);
            std::string file_ext(file.extension);

            t(file_name, file_ext);
        }

        tinydir_next(&close.dir);
    }
}

void font_selector::reset_default_fonts(float editor_font_size)
{
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;

    ImFontConfig font_cfg;
    font_cfg.GlyphExtraSpacing = ImVec2(char_inf::extra_glyph_spacing, 0);

    atlas->FontBuilderFlags = fonts_flags;
    font_cfg.FontBuilderFlags = fonts_flags;

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->Clear();
    //io.Fonts->ClearFonts();

    #ifndef __EMSCRIPTEN__
    ///BASE
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", current_base_font_size, &font_cfg);
    io.Fonts->AddFontFromFileTTF("square.ttf", round(current_base_font_size*0.7), &font_cfg);
    #endif // __EMSCRIPTEN__
    ///TEXT_EDITOR
    //io.Fonts->AddFontFromFileTTF("VeraMono.ttf", editor_font_size, &font_cfg);
    ///DEFAULT
    io.Fonts->AddFontDefault();

    #ifdef __EMSCRIPTEN__
    io.Fonts->AddFontDefault(); ///kinda hacky
    #endif // __EMSCRIPTEN__

    #ifndef __EMSCRIPTEN__
    for_each_file("./", [&](const std::string& name, const std::string& ext)
    {
        if(ext != "ttf")
            return;

        if(name == "square.ttf" || name == "VeraMono.ttf")
            return;

        io.Fonts->AddFontFromFileTTF(name.c_str(), current_base_font_size, &font_cfg);
    });
    #endif // __EMSCRIPTEN__

    wants_rebuild = true;

    ImGui::GetIO().Fonts->Build();

    unsigned char* out_pix = nullptr;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&out_pix, nullptr, nullptr, nullptr);
}

// Call _BEFORE_ NewFrame()
bool font_selector::update_rebuild(float editor_font_size)
{
    #if 0
    if (!wants_rebuild)
        return false;

    win.pushGLStates();

    reset_default_fonts(editor_font_size);

    ImGuiIO& io = ImGui::GetIO();

    io.FontDefault = io.Fonts->Fonts[current_base_font];

    for (int n = 0; n < io.Fonts->Fonts.Size; n++)
    {
        //io.Fonts->Fonts[n]->ConfigData->RasterizerMultiply = FontsMultiply;
        //io.Fonts->Fonts[n]->ConfigData->RasterizerFlags = (BuildMode == FontBuildMode_FreeType) ? fonts_flags : 0x00;
    }

    ImFontAtlas* atlas = ImGui::SFML::GetFontAtlas();

    ImGuiFreeType::BuildFontAtlas(atlas, fonts_flags, subpixel_flags);

    wants_rebuild = false;

    auto write_data =  [](unsigned char* data, void* tex_id, int width, int height)
    {
        sf::Texture* tex = (sf::Texture*)tex_id;

        tex->create(width, height);
        tex->update((const unsigned char*)data);
    };


    ImGuiFreeType::BuildFontAtlas(atlas, ImGuiFreeType::ForceAutoHint, ImGuiFreeType::LEGACY);

    write_data((unsigned char*)atlas->TexPixelsNewRGBA32, (void*)&font_atlas, atlas->TexWidth, atlas->TexHeight);

    atlas->TexID = (void*)font_atlas.getNativeHandle();

    win.popGLStates();

    return true;
    #endif // 0

    if(!wants_rebuild)
        return false;

    reset_default_fonts(current_base_font_size);
    wants_rebuild = false;

    return true;
}

// Call to draw interface
void font_selector::render(render_window& window)
{
    if(!is_open)
        return;

    ImGui::Begin("FreeType Options", &is_open, ImGuiWindowFlags_AlwaysAutoResize);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font_current = ImGui::GetFont();

    if(ImGui::BeginCombo("Fonts", font_current->GetDebugName()))
    {
        for(int n = 0; n < io.Fonts->Fonts.Size; n++)
        {
            if(ImGui::Selectable(io.Fonts->Fonts[n]->GetDebugName(), io.Fonts->Fonts[n] == font_current))
            {
                //io.FontDefault = io.Fonts->Fonts[n];
                wants_rebuild = true;
                current_base_font = n;
                current_font_name = io.Fonts->Fonts[n]->GetDebugName();
            }
        }
        ImGui::EndCombo();
    }

    //wants_rebuild |= ImGui::DragFloat("Multiply", &fonts_multiply, 0.001f, 0.0f, 2.0f);

    wants_rebuild |= ImGui::CheckboxFlags("NoHinting",     &fonts_flags, ImGuiFreeTypeBuilderFlags_NoHinting);
    wants_rebuild |= ImGui::CheckboxFlags("NoAutoHint",    &fonts_flags, ImGuiFreeTypeBuilderFlags_NoAutoHint);
    wants_rebuild |= ImGui::CheckboxFlags("ForceAutoHint", &fonts_flags, ImGuiFreeTypeBuilderFlags_ForceAutoHint);
    wants_rebuild |= ImGui::CheckboxFlags("LightHinting",  &fonts_flags, ImGuiFreeTypeBuilderFlags_LightHinting);
    wants_rebuild |= ImGui::CheckboxFlags("MonoHinting",   &fonts_flags, ImGuiFreeTypeBuilderFlags_MonoHinting);
    wants_rebuild |= ImGui::CheckboxFlags("Bold",          &fonts_flags, ImGuiFreeTypeBuilderFlags_Bold);
    wants_rebuild |= ImGui::CheckboxFlags("Oblique",       &fonts_flags, ImGuiFreeTypeBuilderFlags_Oblique);

    ImGui::NewLine();

    wants_rebuild |= ImGui::CheckboxFlags("Default Filtering", &fonts_flags, ImGuiFreeTypeBuilderFlags_FILTER_DEFAULT);
    wants_rebuild |= ImGui::CheckboxFlags("Legacy Filtering", &fonts_flags, ImGuiFreeTypeBuilderFlags_FILTER_LEGACY);
    wants_rebuild |= ImGui::CheckboxFlags("Light Filtering", &fonts_flags, ImGuiFreeTypeBuilderFlags_FILTER_LIGHT);
    wants_rebuild |= ImGui::CheckboxFlags("No Filtering", &fonts_flags, ImGuiFreeTypeBuilderFlags_FILTER_NONE);
    wants_rebuild |= ImGui::CheckboxFlags("Disable subpixel AA", &fonts_flags, ImGuiFreeTypeBuilderFlags_NO_SUBPIXEL_AA);

    ImGui::NewLine();

    auto sett = window.get_render_settings();

    if(ImGui::Checkbox("SrgbFramebuffer", &sett.is_srgb))
    {
        window.set_srgb(sett.is_srgb);
    }

    if(ImGui::Checkbox("Vsync Enabled", &sett.vsync))
    {
        window.set_vsync(sett.vsync);
    }

    ImGui::NewLine();

    ImGui::NewLine();

    int ifsize = current_base_font_size;

    if(ImGui::DragInt("Font Size", &ifsize, 1, 5, 26))
    {
        wants_rebuild = true;
    }

    if(ifsize < 5)
        ifsize = 5;
    if(ifsize > 26)
        ifsize = 26;

    ImGui::SameLine();

    if(ImGui::Button("-"))
    {
        ifsize--;
        wants_rebuild = true;
    }

    ImGui::SameLine();

    if(ImGui::Button("+"))
    {
        ifsize++;
        wants_rebuild = true;
    }

    current_base_font_size = ifsize;

    if(wants_rebuild)
    {
        file::write("font_sett_v1.txt", ::serialise(*this, serialise_mode::DISK).dump(), file::mode::TEXT);
    }

    ImGui::End();
}
