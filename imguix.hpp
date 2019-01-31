#ifndef IMGUIX_HPP_INCLUDED
#define IMGUIX_HPP_INCLUDED

#include <imgui/imgui.h>
#include <string>
#include <vec/vec.hpp>
#include "imgui_internal.h"
#include <map>

namespace ImGuiX
{
    inline
    ImVec4 GetStyleCol(ImGuiCol name)
    {
        auto res = ImGui::GetColorU32(name, 1.f);

        return ImGui::ColorConvertU32ToFloat4(res);
    }

    inline
    void OutlineHoverText(const std::string& txt, vec3f col, vec3f text_col, bool hover = true, vec2f dim_extra = {0,0}, int thickness = 1, bool force_hover = false, vec3f hover_col = {-1, -1, -1}, int force_hover_thickness = 0)
    {
        ImGui::BeginGroup();

        //auto cursor_pos = ImGui::GetCursorPos();
        auto screen_pos = ImGui::GetCursorScreenPos();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(text_col.x(), text_col.y(), text_col.z(), 1));

        auto dim = ImGui::CalcTextSize(txt.c_str());

        dim.x += dim_extra.x();
        dim.y += dim_extra.y();

        dim.y += 2;
        dim.x += 1;

        ImVec2 p2 = screen_pos;
        p2.x += dim.x;
        p2.y += dim.y;

        auto win_bg_col = GetStyleCol(ImGuiCol_WindowBg);

        bool currently_hovered = ImGui::IsWindowHovered() && ImGui::IsRectVisible(dim) && ImGui::IsMouseHoveringRect(screen_pos, p2) && hover;

        if(currently_hovered || force_hover)
        {
            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::PushStyleColor(ImGuiCol_Button, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetStyleCol(ImGuiCol_WindowBg));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::PopStyleColor(3);

            if(force_hover && !currently_hovered)
            {
                thickness = force_hover_thickness;
            }

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x(), col.y(), col.z(), 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col.x(), col.y(), col.z(), 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col.x(), col.y(), col.z(), 1));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::PopStyleColor(3);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x, screen_pos.y));

            auto button_col = GetStyleCol(ImGuiCol_WindowBg);

            if(hover_col.x() != -1)
            {
                button_col = ImVec4(hover_col.x(), hover_col.y(), hover_col.z(), 1);
            }

            ImGui::PushStyleColor(ImGuiCol_Button, button_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_col);

            if((!ImGui::IsMouseDown(0) && currently_hovered) || (force_hover && !currently_hovered))
                ImGui::Button("", dim);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x + dim_extra.x()/2.f, screen_pos.y + dim_extra.y()/2.f));

            ImGui::Text(txt.c_str());
        }
        else
        {
            win_bg_col.w = 0.f;

            ImGui::PushStyleColor(ImGuiCol_Button, win_bg_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, win_bg_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, win_bg_col);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x + dim_extra.x()/2.f, screen_pos.y + dim_extra.y()/2.f));

            ImGui::Text(txt.c_str());
        }

        ImGui::PopStyleColor(4);

        ImGui::EndGroup();
    }

    inline
    void ToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active)
    {
        OutlineHoverText(txt, highlight_col, col, true, {8.f, 2.f}, 1, is_active, highlight_col/4.f, is_active);
    }

    inline
    void SolidToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active)
    {
        if(is_active)
            OutlineHoverText(txt, highlight_col, {1,1,1}, true, {8, 2}, 1, true, highlight_col, 1);
        else
            OutlineHoverText(txt, highlight_col, {1,1,1}, true, {8, 2}, 1, true, highlight_col/4.f, 1);
    }

    void SolidSmallButton(const std::string& txt, vec3f highlight_col, vec3f col, bool force_hover, vec2f dim = {0,0});

    inline
    void OutlineHoverTextAuto(const std::string& txt, vec3f text_col, bool hover = true, vec2f dim_extra = {0,0}, int thickness = 1, bool force_hover = false)
    {
        return OutlineHoverText(txt, text_col/2.f, text_col, hover, dim_extra, thickness, force_hover);
    }

    /*inline
    void TextColored(const std::string& str, vec3f col)
    {
        TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());
    }*/

    inline
    void Text(const std::string& str)
    {
        ImGui::Text(str.c_str());
    }

    inline
    void ClickText(const std::string& label, vec3f col, vec2f dim_extra)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        const ImGuiID id = window->GetID(label.c_str());

        static std::map<ImGuiID, bool> clicked_state;

        //ImGuiX::SolidToggleTextButton(fin, {1, 1, 1}, {1, 1, 1}, false);
        //ImGuiX::OutlineHoverTextAuto(fin.c_str(), {1, 1, 1}, true, {(max_width - width) - 3, 0});

        if(clicked_state[id])
            ImGuiX::OutlineHoverText(label, col, col, true, dim_extra, 1, true, {1, 1, 1}, 1);
        else
            ImGuiX::OutlineHoverText(label, col, col, true, dim_extra, 1, false, (vec3f){1, 1, 1}/4.f, 1);

        if(ImGui::IsItemClicked(0))
        {
            clicked_state[id] = true;
        }

        if(clicked_state[id] && !ImGui::IsMouseDown(0))
        {
            clicked_state[id] = false;
        }
    }
}

#endif // IMGUIX_HPP_INCLUDED
