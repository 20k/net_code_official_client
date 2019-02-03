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

inline
void ImGuiX::OutlineHoverText(const std::string& txt, vec3f col, vec3f text_col, bool hover, vec2f dim_extra, int thickness, bool force_hover, vec3f hover_col, int force_hover_thickness)
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
void ImGuiX::ToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active)
{
    OutlineHoverText(txt, highlight_col, col, true, {8.f, 2.f}, 1, is_active, highlight_col/4.f, is_active);
}

inline
void ImGuiX::SolidToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active)
{
    if(is_active)
        OutlineHoverText(txt, highlight_col, {1,1,1}, true, {8, 2}, 1, true, highlight_col, 1);
    else
        OutlineHoverText(txt, highlight_col, {1,1,1}, true, {8, 2}, 1, true, highlight_col/4.f, 1);
}

void ImGuiX::OutlineHoverTextAuto(const std::string& txt, vec3f text_col, bool hover, vec2f dim_extra, int thickness, bool force_hover)
{
    return OutlineHoverText(txt, text_col/2.f, text_col, hover, dim_extra, thickness, force_hover);
}

/*inline
void TextColored(const std::string& str, vec3f col)
{
    TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());
}*/

void ImGuiX::Text(const std::string& str)
{
    ImGui::Text(str.c_str());
}

bool ImGuiX::ClickText(const std::string& label, vec3f col, vec2f dim_extra)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImGuiID id = window->GetID(label.c_str());

    static std::map<ImGuiID, bool> clicked_state;

    //ImGuiX::SolidToggleTextButton(fin, {1, 1, 1}, {1, 1, 1}, false);
    //ImGuiX::OutlineHoverTextAuto(fin.c_str(), {1, 1, 1}, true, {(max_width - width) - 3, 0});

    if(clicked_state[id])
        ImGuiX::OutlineHoverText(label, col, col, true, dim_extra, 1, true, {1, 1, 1}, 1);
    else
        ImGuiX::OutlineHoverText(label, col, col, true, dim_extra, 1, false, (vec3f)
    {
        1, 1, 1
    }/4.f, 1);

    bool clicked = false;

    if(ImGui::IsItemClicked(0))
    {
        clicked_state[id] = true;
        clicked = true;
    }

    if(clicked_state[id] && !ImGui::IsMouseDown(0))
    {
        clicked_state[id] = false;
    }

    return clicked;
}

std::string ImGuiX::SurroundText(const std::string& in, int idx, int max_idx, int max_len)
{
    std::string ret = "";

    int len = in.size();

    if(max_len > len)
        len = max_len;

    /*std::string top = "/";
    std::string bottom = "\\";

    for(int i=0; i < len; i++)
    {
        top += "=";
        bottom += "=";
    }

    top += "\\";
    bottom += "/";*/

    //std::string middle = "|" + in + "|";

    std::string middle = in;

    int diff = (max_len - (int)in.size());

    int midx = diff / 2;

    if((diff % 2) == 1)
        midx++;

    ///uncomment this to centre adjust
    midx = 0;

    for(int i=0; i < midx; i++)
    {
        middle = " " + middle;
    }

    for(int i=middle.size(); i < len; i++)
    {
        middle = middle + " ";
    }

    middle = "|" + middle + "|";

    std::string full = "";

    /*if(idx == 0)
        full += top + "\n";

    full += middle + "\n";

    if(idx == max_idx - 1 || max_idx == 0)
        full += bottom + "\n";

    return full;*/

    return middle + "\n";
}

int ImGuiX::ClickableList(const std::vector<std::string>& in)
{
    int longest_name = 0;

    for(auto& i : in)
    {
        if((int)i.size() > longest_name)
        {
            longest_name = i.size();
        }
    }

    int max_width = 60;

    int size_idx = 0;

    for(auto& i : in)
    {
        std::string fin = ImGuiX::SurroundText(i, size_idx, in.size(), longest_name);

        int width = ImGui::CalcTextSize(fin.c_str()).x;

        max_width = std::max(width, max_width);

        size_idx++;
    }

    max_width += ImGui::GetStyle().ItemInnerSpacing.x * 2;

    ImGui::BeginChild("left_selector", ImVec2(max_width, 0));

    int button_count = 0;

    int ridx = -1;

    std::string top = "/";
    std::string bottom = "\\";

    for(int i=0; i < longest_name; i++)
    {
        top += "=";
        bottom += "=";
    }

    top += "\\";
    bottom += "/";

    int fudge_x = 3;

    ImGui::SetCursorPosX((max_width - ImGui::CalcTextSize(top.c_str()).x) - fudge_x);

    ImGuiX::Text(top);

    for(auto& i : in)
    {
        std::string fin = ImGuiX::SurroundText(i, button_count, in.size(), longest_name);

        int width = ImGui::CalcTextSize(fin.c_str()).x;

        ImGui::SetCursorPosX(1);

        vec2f dim_extra = {(max_width - width) - fudge_x, 0};

        if(ImGuiX::ClickText(fin, {1,1,1}, dim_extra))
        {
            ridx = button_count;
        }

        button_count++;
    }

    ImGui::SetCursorPosX((max_width - ImGui::CalcTextSize(bottom.c_str()).x) - fudge_x);

    ImGuiX::Text(bottom);

    ImGui::EndChild();

    return ridx;
}

void ImGuiX::BeginCustomEmbedded(const std::string& title, const std::string& identifier)
{
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGuiX::GetBgCol());
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGuiX::GetBgCol());
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImGuiX::GetBgCol());

    static vec2f last_size;

    int pad_num = last_size.x() / ImGui::CalcTextSize(" ").x;

    std::string to_show = title;

    /*for(int i=0; i < pad_num; i++)
    {
        to_show += "=";
    }*/

    to_show += identifier;

    ImGui::Begin(to_show.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ResizeFromAnySide | ImGuiWindowFlags_MenuBar);

    last_size = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};
}

void ImGuiX::BeginCustomWrapper()
{
    ImVec2 spos = ImGui::GetWindowPos();

    ImGui::BeginChild("customchild");

    int width = ImGui::GetWindowWidth();
    //int height = ImGui::GetWindowHeight();
    //int height = ImGui::GetContentRegionAvail().y;


    int wnum = width / char_inf::cwidth;

    wnum -= 1;

    std::string str = "";

    for(int i=0; i < wnum; i++)
    {
        str += "=";
    }

    ImGui::Text(str.c_str());

    ImVec2 cursor = ImGui::GetCursorPos();

    while(true)
    {
        if(ImGui::GetCursorPosY() + char_inf::cheight >= ImGui::GetWindowHeight())
            break;

        ImGui::Text("|");
    }

    cursor.x += ImGui::CalcTextSize(" ").x;

    ImGui::SetCursorPos(cursor);


    //ImGui::Text("============================================");
}

void ImGuiX::EndCustomEmbedded()
{
    ImGui::EndChild();

    ImGui::End();

    ImGui::PopStyleColor(3);
}
