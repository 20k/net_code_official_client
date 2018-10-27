#include "font_cfg.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui-sfml/imgui-SFML.h>
#include <imgui/misc/freetype/imgui_freetype.h>

#include <SFML/Graphics.hpp>

#include "string_helpers.hpp"
#include <tinydir/tinydir.h>

void update_font_texture_safe()
{
    /*sf::Texture& texture = get_font_atlas();

    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    std::cout << "fwidth " << width << " fheight " << height << std::endl;

    sf::Image bad_pixels;

    bad_pixels = texture.copyToImage();

    const sf::Uint8* bad_pixels_ptr = bad_pixels.getPixelsPtr();

    for(int i=0; i < 100; i++)
    {
        std::cout << "f1 " << (int)bad_pixels_ptr[i] << " f2 " << (int)pixels[i] << std::endl;
    }


    //texture.create(width, height);
    //texture.update(pixels);

    io.Fonts->TexID = reinterpret_cast<void*>(texture.getNativeHandle());*/
}

font_selector::font_selector()
{
    fonts_flags = ImGuiFreeType::ForceAutoHint;
    //fonts_flags = ImGuiFreeType::ForceAutoHint | ImGuiFreeType::MonoHinting;

    sf::Font font;
    font.loadFromFile("VeraMono.ttf");

    sfml_fonts.push_back(font);
}

ImFont* font_selector::get_base_font()
{
    ImGuiIO& io = ImGui::GetIO();

    return io.Fonts->Fonts[current_base_font];
}

ImFont* font_selector::get_editor_font()
{
    ImGuiIO& io = ImGui::GetIO();

    return io.Fonts->Fonts[(int)font_cfg::TEXT_EDITOR];
}

sf::Font* font_selector::get_base_sfml_font()
{
    return &sfml_fonts[0];
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
    ImFontConfig font_cfg;
    font_cfg.GlyphExtraSpacing = ImVec2(char_inf::extra_glyph_spacing, 0);

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->Clear();
    //io.Fonts->ClearFonts();

    ///BASE
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", current_base_font_size, &font_cfg);
    ///TEXT_EDITOR
    io.Fonts->AddFontFromFileTTF("VeraMono.ttf", editor_font_size, &font_cfg);
    ///DEFAULT
    io.Fonts->AddFontDefault();

    for_each_file("./", [&](const std::string& name, const std::string& ext)
    {
        if(ext != "ttf")
            return;

        io.Fonts->AddFontFromFileTTF(name.c_str(), current_base_font_size, &font_cfg);
    });

    //ImGui::PushFont(font);

    wants_rebuild = true;
}

// Call _BEFORE_ NewFrame()
bool font_selector::update_rebuild(sf::RenderWindow& win, float editor_font_size)
{
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


    ImGuiFreeType::BuildFontAtlas(font_atlas, io.Fonts, fonts_flags, subpixel_flags);

    wants_rebuild = false;
    update_font_texture_safe();

    win.popGLStates();

    //io.Fonts->TexID = reinterpret_cast<void*>(backing.getNativeHandle());

    //ImGui::PushFont(io.Fonts->Fonts[0]);
    return true;
}

bool handle_checkbox(const std::vector<std::string>& in, unsigned int& storage, const std::vector<int>& to_set)
{
    bool any = false;

    for(int i=0; i < (int)in.size(); i++)
    {
        bool is_set = to_set[i] == storage;

        any |= ImGui::Checkbox(in[i].c_str(), &is_set);

        if(is_set)
        {
            storage = to_set[i];
        }
    }

    return any;
}

// Call to draw interface
void font_selector::render(window_context& window_ctx)
{
    if(!is_open)
        return;

    ImGui::Begin("FreeType Options", &is_open, ImGuiWindowFlags_AlwaysAutoResize);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font_current = ImGui::GetFont();

    if (ImGui::BeginCombo("Fonts", font_current->GetDebugName()))
    {
        for (int n = 0; n < io.Fonts->Fonts.Size; n++)
        {
            if (ImGui::Selectable(io.Fonts->Fonts[n]->GetDebugName(), io.Fonts->Fonts[n] == font_current))
            {
                //io.FontDefault = io.Fonts->Fonts[n];
                wants_rebuild = true;
                current_base_font = n;
            }
        }
        ImGui::EndCombo();
    }

    wants_rebuild |= ImGui::DragFloat("Multiply", &fonts_multiply, 0.001f, 0.0f, 2.0f);

    wants_rebuild |= ImGui::CheckboxFlags("NoHinting",     &fonts_flags, ImGuiFreeType::NoHinting);
    wants_rebuild |= ImGui::CheckboxFlags("NoAutoHint",    &fonts_flags, ImGuiFreeType::NoAutoHint);
    wants_rebuild |= ImGui::CheckboxFlags("ForceAutoHint", &fonts_flags, ImGuiFreeType::ForceAutoHint);
    wants_rebuild |= ImGui::CheckboxFlags("LightHinting",  &fonts_flags, ImGuiFreeType::LightHinting);
    wants_rebuild |= ImGui::CheckboxFlags("MonoHinting",   &fonts_flags, ImGuiFreeType::MonoHinting);
    wants_rebuild |= ImGui::CheckboxFlags("Bold",          &fonts_flags, ImGuiFreeType::Bold);
    wants_rebuild |= ImGui::CheckboxFlags("Oblique",       &fonts_flags, ImGuiFreeType::Oblique);

    ImGui::NewLine();

    /*wants_rebuild |= ImGui::CheckboxFlags("Default Filtering", &subpixel_flags, ImGuiFreeType::DEFAULT);
    wants_rebuild |= ImGui::CheckboxFlags("Legacy Filtering", &subpixel_flags, ImGuiFreeType::LEGACY);
    wants_rebuild |= ImGui::CheckboxFlags("Light Filtering", &subpixel_flags, ImGuiFreeType::LIGHT);
    wants_rebuild |= ImGui::CheckboxFlags("No Filtering", &subpixel_flags, ImGuiFreeType::NONE);
    wants_rebuild |= ImGui::CheckboxFlags("Disable subpixel AA", &subpixel_flags, ImGuiFreeType::DISABLE_SUBPIXEL_AA);*/

    wants_rebuild |= handle_checkbox({"Default Filtering", "Legacy Filtering", "Light Filtering", "No Filtering", "Disable subpixel AA"},
                                    subpixel_flags,
                                    {ImGuiFreeType::DEFAULT, ImGuiFreeType::LEGACY, ImGuiFreeType::LIGHT, ImGuiFreeType::NONE, ImGuiFreeType::DISABLE_SUBPIXEL_AA});

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
        serialise sfont;
        sfont.handle_serialise(*this, true);
        sfont.save("./font_sett.txt");
    }

    ImGui::End();
}
