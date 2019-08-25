#include "imguix.hpp"
#include "imgui_internal.h"
#include <map>
#include "string_helpers.hpp"

ImVec4 ImGuiX::GetBgCol()
{
    vec3f bg_col = {30, 30, 30};

    return ImVec4(bg_col.x()/255.f, bg_col.y()/255.f, bg_col.z()/255.f, 255/255.f);
}

ImVec4 ImGuiX::GetStyleCol(ImGuiCol name)
{
    auto res = ImGui::GetColorU32(name, 1.f);

    return ImGui::ColorConvertU32ToFloat4(res);
}

void ImGuiX::Text(const std::string& str)
{
    ImGui::Text(str.c_str());
}
