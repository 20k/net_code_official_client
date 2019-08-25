#ifndef IMGUIX_HPP_INCLUDED
#define IMGUIX_HPP_INCLUDED

#include <imgui/imgui.h>
#include <string>
#include <vec/vec.hpp>

namespace ImGuiX
{
    ImVec4 GetBgCol();

    ImVec4 GetStyleCol(ImGuiCol name);
    void Text(const std::string& str);
}

#endif // IMGUIX_HPP_INCLUDED
