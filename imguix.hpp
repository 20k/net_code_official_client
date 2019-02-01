#ifndef IMGUIX_HPP_INCLUDED
#define IMGUIX_HPP_INCLUDED

#include <imgui/imgui.h>
#include <string>
#include <vec/vec.hpp>

namespace ImGuiX
{
    ImVec4 GetStyleCol(ImGuiCol name);
    void OutlineHoverText(const std::string& txt, vec3f col, vec3f text_col, bool hover = true, vec2f dim_extra = {0,0}, int thickness = 1, bool force_hover = false, vec3f hover_col = {-1, -1, -1}, int force_hover_thickness = 0);
    void ToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active);

    void SolidToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active);
    void SolidSmallButton(const std::string& txt, vec3f highlight_col, vec3f col, bool force_hover, vec2f dim = {0,0});

    void OutlineHoverTextAuto(const std::string& txt, vec3f text_col, bool hover = true, vec2f dim_extra = {0,0}, int thickness = 1, bool force_hover = false);

    /*inline
    void TextColored(const std::string& str, vec3f col)
    {
        TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());
    }*/

    void Text(const std::string& str);

    bool ClickText(const std::string& label, vec3f col, vec2f dim_extra);

    std::string SurroundText(const std::string& in, int idx, int max_idx, int max_len);

    int ClickableList(const std::vector<std::string>& in);
}

#endif // IMGUIX_HPP_INCLUDED
