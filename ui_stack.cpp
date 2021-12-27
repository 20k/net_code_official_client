#include "ui_stack.hpp"
#include <networking/networking.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

std::string get_element_id(const std::string& type, const std::vector<nlohmann::json>& data)
{
    if(type == "text")
        return data.at(0);

    if(type == "textcolored")
        return data.at(4);

    if(type == "textdisabled")
        return data.at(0);

    if(type == "bullettext")
        return data.at(0);

    if(type == "button" || type == "smallbutton" || type == "invisiblebutton" || type == "arrowbutton" || type == "checkbox")
        return data.at(0);

    if(type == "dragfloat" || type == "dragfloat2" || type == "dragfloat3" || type == "dragfloat4"
       || type == "sliderfloat" || type == "sliderfloat2" || type == "sliderfloat3" || type == "sliderfloat4"
       || type == "sliderangle"
       || type == "sliderint" || type == "sliderint2" || type == "sliderint3" || type == "sliderint4"
       || type == "dragint" || type == "dragint2" || type == "dragint3" || type == "dragint4")
        return data.at(0);

    if(type == "inputtext" || type == "inputtextmultiline"
    || type == "inputint" || type == "inputint2" || type == "inputint3" || type == "inputint4"
    || type == "inputfloat" || type == "inputfloat2" || type == "inputfloat3" || type == "inputfloat4"
    || type == "inputdouble")
        return data.at(0);

    if(type == "endgroup")
        return data.at(0);

    if(type == "coloredit3" || type == "coloredit4"
    || type == "colorpicker3" || type == "colorpicker4"
    || type == "colorbutton")
        return data.at(0);

    if(type == "treenode" || type == "treepush" || type == "collapsingheader")
        return data.at(0);

    if(type == "selectable")
        return data.at(0);

    if(type == "listbox")
        return data.at(0);

    if(type == "plotlines" || type == "plothistogram")
        return data.at(0);

    if(type == "begindragdropsource" || type == "begindragdroptarget" || type == "acceptdragdroppayload")
        return data.at(0);

    return "";
}

template<typename T, int N>
std::optional<nlohmann::json> dragTN(ui_element& e)
{
    std::array<T, N> my_vals;

    static_assert(std::is_same_v<T, float> || std::is_same_v<T, int>);

    std::string id = e.arguments[0];

    for(int i=0; i < N; i++)
    {
        my_vals[i] = e.arguments[i + 1];
    }

    std::array prev_vals = my_vals;

    float v_speed = e.arguments[N + 1];
    float v_min = e.arguments[N + 2];
    float v_max = e.arguments[N + 3];

    if constexpr(std::is_same_v<T, float>)
    {
        if(N == 1)
            ImGui::DragFloat(id.c_str(), &my_vals[0], v_speed, v_min, v_max);

        if(N == 2)
            ImGui::DragFloat2(id.c_str(), &my_vals[0], v_speed, v_min, v_max);

        if(N == 3)
            ImGui::DragFloat3(id.c_str(), &my_vals[0], v_speed, v_min, v_max);

        if(N == 4)
            ImGui::DragFloat4(id.c_str(), &my_vals[0], v_speed, v_min, v_max);
    }

    if constexpr(std::is_same_v<T, int>)
    {
        if(N == 1)
            ImGui::DragInt(id.c_str(), &my_vals[0], v_speed, v_min, v_max);

        if(N == 2)
            ImGui::DragInt2(id.c_str(), &my_vals[0], v_speed, v_min, v_max);

        if(N == 3)
            ImGui::DragInt3(id.c_str(), &my_vals[0], v_speed, v_min, v_max);

        if(N == 4)
            ImGui::DragInt4(id.c_str(), &my_vals[0], v_speed, v_min, v_max);
    }

    for(int i=0; i < N; i++)
    {
        e.arguments[i + 1] = my_vals[i];
    }

    if(my_vals != prev_vals)
    {
        return my_vals;
    }

    return std::nullopt;
}

struct angle_tag{};

template<typename T, int N, typename tag = T>
std::optional<nlohmann::json> sliderTN(ui_element& e)
{
    std::array<T, N> my_vals;

    static_assert(std::is_same_v<T, float> || std::is_same_v<T, int>);
    static_assert(std::is_same_v<tag, float> || std::is_same_v<tag, int> || std::is_same_v<tag, angle_tag>);

    std::string id = e.arguments[0];

    for(int i=0; i < N; i++)
    {
        my_vals[i] = e.arguments[i + 1];
    }

    std::array prev_vals = my_vals;

    float v_min = e.arguments[N + 1];
    float v_max = e.arguments[N + 2];

    if constexpr(std::is_same_v<tag, float>)
    {
        if(N == 1)
            ImGui::SliderFloat(id.c_str(), &my_vals[0], v_min, v_max);

        if(N == 2)
            ImGui::SliderFloat2(id.c_str(), &my_vals[0], v_min, v_max);

        if(N == 3)
            ImGui::SliderFloat3(id.c_str(), &my_vals[0], v_min, v_max);

        if(N == 4)
            ImGui::SliderFloat4(id.c_str(), &my_vals[0], v_min, v_max);
    }

    if constexpr(std::is_same_v<tag, int>)
    {
        if(N == 1)
            ImGui::SliderInt(id.c_str(), &my_vals[0], v_min, v_max);

        if(N == 2)
            ImGui::SliderInt2(id.c_str(), &my_vals[0], v_min, v_max);

        if(N == 3)
            ImGui::SliderInt3(id.c_str(), &my_vals[0], v_min, v_max);

        if(N == 4)
            ImGui::SliderInt4(id.c_str(), &my_vals[0], v_min, v_max);
    }

    if constexpr(std::is_same_v<tag, angle_tag>)
    {
        if(N == 1)
            ImGui::SliderAngle(id.c_str(), &my_vals[0], v_min, v_max);
    }

    for(int i=0; i < N; i++)
    {
        e.arguments[i + 1] = my_vals[i];
    }

    if(my_vals != prev_vals)
    {
        return my_vals;
    }

    return std::nullopt;
}

struct rawtext_tag{};
struct multiline_tag{};

template<typename T, int N, typename tag = T>
std::optional<nlohmann::json> inputTN(ui_element& e)
{
    std::array<T, N> my_vals;

    static_assert(std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<tag, double> || std::is_same_v<T, std::string>);
    static_assert(std::is_same_v<tag, float> || std::is_same_v<tag, int> || std::is_same_v<tag, double> || std::is_same_v<tag, rawtext_tag> || std::is_same_v<tag, multiline_tag>);

    std::string id = e.arguments[0];

    for(int i=0; i < N; i++)
    {
        my_vals[i] = e.arguments[i + 1];
    }

    std::array prev_vals = my_vals;

    if constexpr(std::is_same_v<tag, rawtext_tag>)
    {
        if(N == 1)
        {
            ImGui::InputText(id.c_str(), &my_vals[0]);
        }
    }

    if constexpr(std::is_same_v<tag, multiline_tag>)
    {
        if(N == 1)
        {
            ImGui::InputTextMultiline(id.c_str(), &my_vals[0]);
        }
    }

    if constexpr(std::is_same_v<tag, float>)
    {
        ImGui::InputScalarN(id.c_str(), ImGuiDataType_Float, &my_vals[0], N, nullptr, nullptr, "%.3f");
    }

    if constexpr(std::is_same_v<tag, int>)
    {
        ImGui::InputScalarN(id.c_str(), ImGuiDataType_S32, &my_vals[0], N, nullptr, nullptr, "%d");
    }

    if constexpr(std::is_same_v<tag, double>)
    {
        ImGui::InputDouble(id.c_str(), &my_vals[0]);
    }

    for(int i=0; i < N; i++)
    {
        e.arguments[i + 1] = my_vals[i];
    }

    if(my_vals != prev_vals)
    {
        return my_vals;
    }

    return std::nullopt;
}

template<int N>
std::optional<nlohmann::json> colorTN(ui_element& e)
{
    std::array<float, N> my_vals = {};
    std::string id = e.arguments[0];

    for(int i=0; i < N; i++)
    {
        my_vals[i] = e.arguments[i + 1];
    }

    std::array prev_vals = my_vals;

    if(e.type == "coloredit3")
    {
        ImGui::ColorEdit3(id.c_str(), &my_vals[0], 0);
    }

    if(e.type == "coloredit4")
    {
        ImGui::ColorEdit4(id.c_str(), &my_vals[0], 0);
    }

    if(e.type == "colorpicker3")
    {
        ImGui::ColorPicker3(id.c_str(), &my_vals[0], 0);
    }

    if(e.type == "colorpicker4")
    {
        ImGui::ColorPicker4(id.c_str(), &my_vals[0], 0);
    }

    for(int i=0; i < N; i++)
    {
        e.arguments[i + 1] = my_vals[i];
    }

    if(my_vals != prev_vals)
    {
        return my_vals;
    }

    return std::nullopt;
}

template<auto Popfunc>
struct safe_item_stack
{
    int stack_size = 0;

    void push()
    {
        stack_size++;
    }

    void pop()
    {
        if(stack_size == 0)
            return;

        stack_size--;
        Popfunc();
    }

    void pop_all()
    {
        while(stack_size > 0)
        {
            stack_size--;
            Popfunc();
        }
    }
};

static void imgui_pop_style_colour_1()
{
    ImGui::PopStyleColor(1);
}

///all values from the server are sanitised in some way unless explicitly noted otherwise
///that is: randomised salted hashes in the strings to prevent collisions
///strings have a capped length
///doubles are not nan, not that json supports that anyway
///all values are clamped to sensible ranges so they can be piped directly into imgui
///colours are clamped to [0, 1]
void render_ui_stack(connection_send_data& to_write, uint64_t& current_sequence_id, ui_stack& stk, int id, bool is_linear_colour)
{
    ImGui::BeginGroup();

    safe_item_stack<ImGui::EndGroup> group_stack;
    safe_item_stack<imgui_pop_style_colour_1> colour_stack;
    safe_item_stack<ImGui::PopItemWidth> item_width_stack;
    //safe_item_stack<ImGui::TreePop> tree_stack;
    bool skipping_ui_elements = false;

    int drag_drop_payload_stack = 0;

    ///would sure love to use std::vector<bool> here
    std::vector<int> tree_indent_stack;
    std::vector<int> active_drag_drop_source_stack;

    for(ui_element& e : stk.elements)
    {
        if(skipping_ui_elements)
        {
            if(e.type != "treepop")
                continue;
        }

        if(active_drag_drop_source_stack.size() > 0 && !active_drag_drop_source_stack.back())
        {
            if(e.type != "enddragdropsource")
                continue;
        }

        bool returns_true = false;
        bool returns_true_dirty = false;

        //if(e.arguments.size() < get_argument_count(e.type))
        //    continue;

        bool buttonbehaviour = false;
        std::optional<nlohmann::json> button_behaviour_dirty_arguments_opt;

        if(e.type == "text")
        {
            std::string val = e.arguments[0];

            ImGui::TextUnformatted(val.c_str(), val.c_str() + val.size());
        }

        if(e.type == "textcolored")
        {
            ///r, g, b, a, text
            float r = e.arguments[0];
            float g = e.arguments[1];
            float b = e.arguments[2];
            float a = e.arguments[3];

            if(is_linear_colour)
            {
                r = srgb_to_lin_approx((vec1f)r).x();
                g = srgb_to_lin_approx((vec1f)g).x();
                b = srgb_to_lin_approx((vec1f)b).x();
                a = srgb_to_lin_approx((vec1f)a).x();
            }

            std::string val = e.arguments[4];

            ImGui::TextColored(ImVec4(r, g, b, a), "%s", val.c_str());
        }

        if(e.type == "textdisabled")
        {
            std::string val = e.arguments[0];

            ImGui::TextDisabled("%s", val.c_str());
        }

        if(e.type == "bullettext")
        {
            std::string val = e.arguments[0];

            ImGui::BulletText("%s", val.c_str());
        }

        if(e.type == "button" || e.type == "smallbutton" || e.type == "invisiblebutton" || e.type == "arrowbutton" || e.type == "checkbox" || e.type == "radiobutton" || e.type == "colorbutton")
        {
            std::string val = e.arguments[0];

            if(e.type == "button")
            {
                double w = e.arguments[1];
                double h = e.arguments[2];

                ImGui::Button(val.c_str(), ImVec2(w, h));
            }

            if(e.type == "smallbutton")
                ImGui::SmallButton(val.c_str());

            if(e.type == "invisiblebutton")
            {
                double w = e.arguments[1];
                double h = e.arguments[2];

                ImGui::InvisibleButton(val.c_str(), ImVec2(w, h));
            }

            if(e.type == "arrowbutton")
            {
                int dir = e.arguments[1];

                ImGui::ArrowButton(val.c_str(), dir);
            }

            if(e.type == "checkbox")
            {
                int arg_as_int = e.arguments[1];
                bool as_bool = arg_as_int;
                bool prev = as_bool;

                ImGui::Checkbox(val.c_str(), &as_bool);

                e.arguments[1] = as_bool;

                if(as_bool != prev)
                {
                    button_behaviour_dirty_arguments_opt = nlohmann::json::array({as_bool});
                }
            }

            if(e.type == "radiobutton")
            {
                ImGui::RadioButton(val.c_str(), (int)e.arguments[1]);
            }

            if(e.type == "colorbutton")
            {
                ImGui::ColorButton(val.c_str(), ImVec4(e.arguments[1], e.arguments[2], e.arguments[3], e.arguments[4]), 0, ImVec2(e.arguments[6], e.arguments[7]));
            }

            buttonbehaviour = true;
        }

        if(e.type == "dragfloat" || e.type == "dragfloat2" || e.type == "dragfloat3" || e.type == "dragfloat4"
           || e.type == "dragint" || e.type == "dragint2" || e.type == "dragint3" || e.type == "dragint4"
           || e.type == "sliderfloat" || e.type == "sliderfloat2" || e.type == "sliderfloat3" || e.type == "sliderfloat4"
           || e.type == "sliderangle"
           || e.type == "sliderint" || e.type == "sliderint2" || e.type == "sliderint3" || e.type == "sliderint4"
           || e.type == "inputtext" || e.type == "inputtextmultiline"
           || e.type == "inputfloat" || e.type == "inputint" || e.type == "inputdouble"
           || e.type == "inputfloat2" || e.type == "inputint2"
           || e.type == "inputfloat3" || e.type == "inputint3"
           || e.type == "inputfloat4" || e.type == "inputfloat4"
           || e.type == "coloredit3" || e.type == "coloredit4"
           || e.type == "colorpicker3" || e.type == "colorpicker4")
        {
            std::string ui_id = e.arguments[0];

            std::optional<nlohmann::json> dirty_arguments_opt;

            if(e.type == "dragfloat")
            {
                dirty_arguments_opt = dragTN<float, 1>(e);
            }

            if(e.type == "dragfloat2")
            {
                dirty_arguments_opt = dragTN<float, 2>(e);
            }

            if(e.type == "dragfloat3")
            {
                dirty_arguments_opt = dragTN<float, 3>(e);
            }

            if(e.type == "dragfloat4")
            {
                dirty_arguments_opt = dragTN<float, 4>(e);
            }

            if(e.type == "dragint")
            {
                dirty_arguments_opt = dragTN<int, 1>(e);
            }

            if(e.type == "dragint2")
            {
                dirty_arguments_opt = dragTN<int, 2>(e);
            }

            if(e.type == "dragint3")
            {
                dirty_arguments_opt = dragTN<int, 3>(e);
            }

            if(e.type == "dragint4")
            {
                dirty_arguments_opt = dragTN<int, 4>(e);
            }

            if(e.type == "sliderfloat")
            {
                dirty_arguments_opt = sliderTN<float, 1>(e);
            }

            if(e.type == "sliderfloat2")
            {
                dirty_arguments_opt = sliderTN<float, 2>(e);
            }

            if(e.type == "sliderfloat3")
            {
                dirty_arguments_opt = sliderTN<float, 3>(e);
            }

            if(e.type == "sliderfloat4")
            {
                dirty_arguments_opt = sliderTN<float, 4>(e);
            }

            if(e.type == "sliderangle")
            {
                dirty_arguments_opt = sliderTN<float, 1, angle_tag>(e);
            }

            if(e.type == "sliderint")
            {
                dirty_arguments_opt = sliderTN<int, 1>(e);
            }

            if(e.type == "sliderint2")
            {
                dirty_arguments_opt = sliderTN<int, 2>(e);
            }

            if(e.type == "sliderint3")
            {
                dirty_arguments_opt = sliderTN<int, 3>(e);
            }

            if(e.type == "sliderint4")
            {
                dirty_arguments_opt = sliderTN<int, 4>(e);
            }

            if(e.type == "inputtext")
            {
                dirty_arguments_opt = inputTN<std::string, 1, rawtext_tag>(e);
            }

            if(e.type == "inputtextmultiline")
            {
                dirty_arguments_opt = inputTN<std::string, 1, multiline_tag>(e);
            }

            if(e.type == "inputfloat")
            {
                dirty_arguments_opt = inputTN<float, 1>(e);
            }

            if(e.type == "inputint")
            {
                dirty_arguments_opt = inputTN<int, 1>(e);
            }

            if(e.type == "inputdouble")
            {
                dirty_arguments_opt = inputTN<double, 1>(e);
            }

            if(e.type == "inputfloat2")
            {
                dirty_arguments_opt = inputTN<float, 2>(e);
            }

            if(e.type == "inputint2")
            {
                dirty_arguments_opt = inputTN<int, 2>(e);
            }

            if(e.type == "inputfloat3")
            {
                dirty_arguments_opt = inputTN<float, 3>(e);
            }

            if(e.type == "inputint3")
            {
                dirty_arguments_opt = inputTN<int, 3>(e);
            }

            if(e.type == "inputfloat4")
            {
                dirty_arguments_opt = inputTN<float, 4>(e);
            }

            if(e.type == "inputint4")
            {
                dirty_arguments_opt = inputTN<int, 4>(e);
            }

            if(e.type == "coloredit3")
            {
                dirty_arguments_opt = colorTN<3>(e);
            }

            if(e.type == "coloredit4")
            {
                dirty_arguments_opt = colorTN<4>(e);
            }

            if(e.type == "colorpicker3")
            {
                dirty_arguments_opt = colorTN<3>(e);
            }

            if(e.type == "colorpicker4")
            {
                dirty_arguments_opt = colorTN<4>(e);
            }

            button_behaviour_dirty_arguments_opt = dirty_arguments_opt;
            buttonbehaviour = true;
        }

        if(e.type == "setnextitemopen")
        {
            ImGui::SetNextItemOpen((int)e.arguments[0]);
        }

        if(e.type == "treenode" || e.type == "treepush" || e.type == "collapsingheader")
        {
            /**
            so the problem is... can't use actual treenode, because it requires calling treepop on true
            the server is the one that makes that decision though
            also don't want to mimic the
            if(ImGui::TreeNode(id))
            {
                ImGui::TreePop();
            }

            structure, because otherwise there'll be a server roundtrip to get new elements in

            A more desirable API is

            ImGui::TreeNode(id)
            ImGui::whatever();
            ImGui::TreePop();

            which can additionally be used like

            if(ImGui::TreeNode(id))
            {
                ImGui::whatever();
            }

            ImGui::TreePop();

            For network bandwidth reasons at the expense of latency*/

            std::string str_id = e.arguments[0];

            bool last_state = e.last_treenode_state;

            e.last_treenode_state = ImGui::CollapsingHeader(str_id.c_str());

            ///collapsingheader never indents, and does not require treepop. treepush always indents. treenode sometimes indents
            if(e.type != "collapsingheader")
            {
                if(e.last_treenode_state || e.type == "treepush")
                {
                    ImGui::Indent();
                    tree_indent_stack.push_back(1);
                }
                else
                {
                    tree_indent_stack.push_back(0);
                }
            }

            ///collapsingheader does not skip elements until treepop is hit
            if(!e.last_treenode_state && e.type != "collapsingheader")
            {
                skipping_ui_elements = true;
            }

            if(e.last_treenode_state != last_state)
            {
                returns_true_dirty = true;
            }

            returns_true = e.last_treenode_state;
            buttonbehaviour = true;
        }

        if(e.type == "treepop")
        {
            skipping_ui_elements = false;

            if(tree_indent_stack.size() > 0)
            {
                if(tree_indent_stack.back() == 1)
                {
                    ImGui::Unindent();
                }

                tree_indent_stack.pop_back();
            }
        }

        if(e.type == "selectable")
        {
            std::string str = e.arguments[0];

            bool is_selected = (int)e.arguments[1];
            bool was_selected = is_selected;

            ImGui::Selectable(str.c_str(), &is_selected, 0, ImVec2(e.arguments[3], e.arguments[4]));

            if(is_selected != was_selected)
            {
                button_behaviour_dirty_arguments_opt = nlohmann::json::array({(int)is_selected});
            }

            e.arguments[1] = (int)is_selected;

            buttonbehaviour = true;
        }

        if(e.type == "listbox")
        {
            std::string str = e.arguments[0];

            int current_item = e.arguments[1];
            int last_current_item = current_item;

            std::vector<std::string> items = e.arguments[2];

            std::vector<const char*> citems;
            citems.resize(items.size());

            for(int i=0; i < (int)items.size(); i++)
            {
                citems[i] = items[i].c_str();
            }

            const char** citems_ptr = nullptr;

            if(citems.size() > 0)
                citems_ptr = &citems[0];

            ImGui::ListBox(str.c_str(), &current_item, citems_ptr, citems.size(), e.arguments[3]);

            if(current_item != last_current_item)
            {
                button_behaviour_dirty_arguments_opt = nlohmann::json::array({current_item});
            }

            e.arguments[1] = current_item;

            buttonbehaviour = true;
        }

        if(e.type == "plotlines" || e.type == "plothistogram")
        {
            std::string str = e.arguments[0];
            std::vector<float> values = e.arguments[1];
            int values_offset = e.arguments[2];
            std::string overlay = e.arguments[3];
            float scale_min = e.arguments[4];
            float scale_max = e.arguments[5];
            ImVec2 graph_size(e.arguments[6], e.arguments[7]);

            float* fptr = nullptr;

            if(values.size() > 0)
                fptr = &values[0];

            if(e.type == "plotlines")
            {
                ImGui::PlotLines(str.c_str(), fptr, values.size(), values_offset, overlay.c_str(), scale_min, scale_max, graph_size);
            }

            if(e.type == "plothistogram")
            {
                ImGui::PlotHistogram(str.c_str(), fptr, values.size(), values_offset, overlay.c_str(), scale_min, scale_max, graph_size);
            }
        }

        if(e.type == "progressbar")
        {
            std::string str = e.arguments[3];

            ImGui::ProgressBar((float)e.arguments[0], ImVec2(e.arguments[1], e.arguments[2]), str.c_str());
        }

        if(e.type == "bullet")
        {
            ImGui::Bullet();
        }

        if(e.type == "pushstylecolor")
        {
            ///IDX IS NOT SANITISED
            int idx = e.arguments[0];
            double r = e.arguments[1];
            double g = e.arguments[2];
            double b = e.arguments[3];
            double a = e.arguments[4];

            if(is_linear_colour)
            {
                r = srgb_to_lin_approx((vec1f)r).x();
                g = srgb_to_lin_approx((vec1f)g).x();
                b = srgb_to_lin_approx((vec1f)b).x();
                a = srgb_to_lin_approx((vec1f)a).x();
            }

            ///PANIC AND BREAK THINGS
            if(idx >= 0 && idx < ImGuiCol_COUNT)
            {
                colour_stack.push();
                ImGui::PushStyleColor(idx, ImVec4(r, g, b, a));
            }
        }

        if(e.type == "popstylecolor")
        {
            int idx = e.arguments[0];

            for(int i=0; i < idx; i++)
            {
                colour_stack.pop();
            }
        }

        if(e.type == "pushitemwidth")
        {
            double width = e.arguments[0];

            ImGui::PushItemWidth(width);
            item_width_stack.push();
        }

        if(e.type == "popitemwidth")
        {
            item_width_stack.pop();
        }

        if(e.type == "setnextitemwidth")
        {
            double width = e.arguments[0];

            ImGui::SetNextItemWidth(width);
        }

        if(e.type == "separator")
        {
            ImGui::Separator();
        }

        if(e.type == "sameline")
        {
            double offset_from_start = e.arguments[0];
            double spacing = e.arguments[1];

            ImGui::SameLine(offset_from_start, spacing);
        }

        if(e.type == "newline")
        {
            ImGui::NewLine();
        }

        if(e.type == "spacing")
        {
            ImGui::Spacing();
        }

        if(e.type == "dummy")
        {
            double w = e.arguments[0];
            double h = e.arguments[1];

            ImGui::Dummy(ImVec2(w, h));
        }

        if(e.type == "indent")
        {
            double amount = e.arguments[0];

            ImGui::Indent(amount);
        }

        if(e.type == "unindent")
        {
            double amount = e.arguments[0];

            ImGui::Unindent(amount);
        }

        if(e.type == "begingroup")
        {
            group_stack.push();
            ImGui::BeginGroup();
        }

        if(e.type == "endgroup")
        {
            if(group_stack.stack_size > 0)
            {
                group_stack.pop();

                buttonbehaviour = true;
            }
        }

        if(e.type == "begindragdropsource")
        {
            bool begindrag = ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID);

            active_drag_drop_source_stack.push_back(begindrag);

            returns_true = begindrag;

            if(begindrag != e.was_return_true)
                returns_true_dirty = true;

            buttonbehaviour = true;
            e.was_return_true = begindrag;
        }

        if(e.type == "setdragdroppayload")
        {
            if(active_drag_drop_source_stack.size() > 0 && active_drag_drop_source_stack.back())
            {
                std::string payload = e.arguments[1];

                if(payload.size() > 0)
                    ImGui::SetDragDropPayload("none", payload.c_str(), payload.size());
                else
                    ImGui::SetDragDropPayload("none", nullptr, 0);
            }
        }

        if(e.type == "enddragdropsource")
        {
            bool last = false;

            if(active_drag_drop_source_stack.size() > 0)
            {
                last = active_drag_drop_source_stack.back();
                active_drag_drop_source_stack.pop_back();
            }

            if(last)
            {
                ImGui::EndDragDropSource();
            }
        }

        if(e.type == "begindragdroptarget")
        {
            bool begindrag = ImGui::BeginDragDropTarget();

            if(begindrag)
            {
                drag_drop_payload_stack++;
            }

            returns_true = begindrag;

            if(begindrag != e.was_return_true)
                returns_true_dirty = true;

            buttonbehaviour = true;
            e.was_return_true = begindrag;
        }

        if(e.type == "acceptdragdroppayload")
        {
            if(drag_drop_payload_stack > 0)
            {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("none");

                if(payload)
                {
                    std::string str;

                    if(payload->DataSize > 0)
                    {
                        str = std::string((char*)payload->Data, (char*)payload->Data + payload->DataSize);
                    }

                    button_behaviour_dirty_arguments_opt = nlohmann::json::array({str});

                    buttonbehaviour = true;
                }
            }
        }

        if(e.type == "enddragdroptarget")
        {
            if(drag_drop_payload_stack > 0)
            {
                ImGui::EndDragDropTarget();

                drag_drop_payload_stack--;
            }
        }

        if(buttonbehaviour && e.element_id != "")
        {
            std::vector<std::string> states;

            bool is_hovered = ImGui::IsItemHovered();
            bool was_hovered = e.was_hovered;

            bool is_clicked = ImGui::IsItemClicked();

            bool is_active = ImGui::IsItemActive();
            bool is_focused = ImGui::IsItemFocused();
            bool is_visible = ImGui::IsItemVisible();
            bool is_edited = ImGui::IsItemEdited();
            bool is_activated = ImGui::IsItemActivated();
            bool is_deactivated = ImGui::IsItemDeactivated();
            bool is_deactivatedafteredit = ImGui::IsItemDeactivatedAfterEdit();
            bool is_toggledopen = ImGui::IsItemToggledSelection(); ///changed to IsItemToggledOpen in later imguis

            if(is_hovered)
                states.push_back("hovered");

            if(is_clicked)
                states.push_back("clicked");

            if(returns_true)
                states.push_back("returnstrue");

            if(is_active)
                states.push_back("active");

            if(is_focused)
                states.push_back("focused");

            if(is_visible)
                states.push_back("visible");

            if(is_edited)
                states.push_back("edited");

            if(is_activated)
                states.push_back("activated");

            if(is_deactivated)
                states.push_back("deactivated");

            if(is_deactivatedafteredit)
                states.push_back("deactivatedafteredit");

            if(is_toggledopen)
                states.push_back("toggledopen");

            if(is_hovered != was_hovered || is_clicked || button_behaviour_dirty_arguments_opt.has_value() || returns_true_dirty ||
               is_active != e.was_active || is_active != e.was_active || is_focused != e.was_focused || is_visible != e.was_visible ||
               is_edited || is_activated || is_deactivated || is_deactivatedafteredit || is_toggledopen
               )
            {
                nlohmann::json j;
                j["type"] = "client_ui_element";
                j["id"] = id;
                j["ui_id"] = e.element_id;
                j["state"] = states;
                j["sequence_id"] = current_sequence_id;

                if(button_behaviour_dirty_arguments_opt.has_value())
                {
                    j["arguments"] = button_behaviour_dirty_arguments_opt.value();
                }

                if(is_clicked || button_behaviour_dirty_arguments_opt.has_value())
                    e.authoritative_until_sequence_id = current_sequence_id;

                write_data dat;
                dat.id = -1;
                dat.data = j.dump();

                to_write.write_to_websocket(std::move(dat));
            }

            e.was_hovered = is_hovered;
            e.was_active = is_active;
            e.was_focused = is_focused;
            e.was_visible = is_visible;
        }
    }

    /*while(in_drag_drop_stack > 0)
    {
        ImGui::EndDragDropSource();
        in_drag_drop_stack--;
    }*/

    for(auto& i : active_drag_drop_source_stack)
    {
        if(i)
        {
            ImGui::EndDragDropSource();
        }
    }

    while(drag_drop_payload_stack > 0)
    {
        ImGui::EndDragDropTarget();
        drag_drop_payload_stack--;
    }

    group_stack.pop_all();

    colour_stack.pop_all();

    item_width_stack.pop_all();

    ImGui::EndGroup();

    current_sequence_id++;
}
