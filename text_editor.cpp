#include "text_editor.hpp"
#include <libncclient/nc_util.hpp>
#include "local_commands.hpp"
#include "font_cfg.hpp"

void editable_script::set_file_name(const std::string& file_name)
{
    ///resetting
    if(has_script)
        save();

    editing_script = file_name;
    has_script = true;

    load();
}

void editable_script::save()
{
    std::string backup = "./scripts/" + editing_script + ".backup";
    std::string temp = "./scripts/" + editing_script + ".temp";
    std::string base = "./scripts/" + editing_script;

    write_all_bin(backup, script_contents);

    remove(temp.c_str());
    rename(base.c_str(), temp.c_str());
    rename(backup.c_str(), base.c_str());
    remove(temp.c_str());

    save_clock.restart();

    force_save = false;
}

void editable_script::load()
{
    script_contents = read_file_bin("./scripts/" + editing_script);
}

void editable_script::tick()
{
    if(!has_script)
        return;

    ///ONLY DO THIS ON MODIFICATIONS
    ///so we can detect if it were externally modified
    if(save_clock.getElapsedTime().asSeconds() > 60 || force_save)
    {
        save();
    }
}

std::string editable_script::get_contents()
{
    return script_contents;
}

void editable_script::set_contents(const std::string& new_contents)
{
    script_contents = new_contents;
}

editable_script::~editable_script()
{
    if(!has_script)
        return;

    save();
}

text_editor_manager::text_editor_manager(font_selector& _font_select) : font_select(_font_select)
{
    auto lang = TextEditor::LanguageDefinition::Lua();
    editor.SetLanguageDefinition(lang);
}

void text_editor_manager::set_current_user(const std::string& username)
{
    editing_user = username;
}

void text_editor_manager::switch_to(int idx)
{
    if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        all_scripts[current_idx].save();
    }

    current_idx = idx;

    if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        editor.SetText(all_scripts[current_idx].get_contents());
    }
    else
    {
        editor.SetText("");
    }
}

void text_editor_manager::close(int idx)
{
    if(idx < 0 || idx >= (int)all_scripts.size())
        return;

    bool should_switch_lower = false;

    if(idx <= current_idx)
    {
        should_switch_lower = true;
    }

    all_scripts[idx].save();

    all_scripts.erase(all_scripts.begin() + idx);

    if(should_switch_lower)
    {
        int tidx = current_idx - 1;

        if(tidx < 0)
            tidx = 0;

        switch_to(tidx);
    }
}

void text_editor_manager::render()
{
    //ImGui::PushFont(font_select.get_editor_font());

    ImGui::Begin("Text Editor", &is_open, ImGuiWindowFlags_MenuBar);

    if(ImGui::IsItemFocused())
    {
        sf::Keyboard key;

        if(key.isKeyPressed(sf::Keyboard::LControl) && ONCE_MACRO(sf::Keyboard::S))
        {
            //force_save = true;

            for(auto& i : all_scripts)
            {
                i.save();
            }
        }
    }

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            /*if (ImGui::MenuItem("Save"))
            {
                auto textToSave = editor.GetText();
                /// save text....
            }
            if (ImGui::MenuItem("Quit", "Alt-F4"))
            {

            }*/

            if(ImGui::BeginMenu("Scripts"))
            {
                std::vector<std::string> script_names = get_scripts_list(editing_user);

                for(auto& name : script_names)
                {
                    //std::string parsed = format_raw_script_name(i);

                    if(ImGui::MenuItem(name.c_str()))
                    {
                        editable_script script;
                        script.set_file_name(name);
                        all_scripts.push_back(script);

                        switch_to((int)all_scripts.size() - 1);

                        any_selected = true;
                    }
                }

                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu("Settings"))
            {
                if(ImGui::Button("-"))
                {
                    current_font_size -= 0.5;
                    dirty_font = true;

                    //font_select.reset_default_fonts(current_font_size);
                }

                ImGui::SameLine();

                std::string str = std::to_string(current_font_size);

                ImGui::Text(str.c_str());

                ImGui::SameLine();

                if(ImGui::Button("+"))
                {
                    current_font_size += 0.5;
                    dirty_font = true;

                    //font_select.reset_default_fonts(current_font_size);
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        for(int i=0; i < (int)all_scripts.size(); i++)
        {
            bool selected = i == current_idx;

            std::string name = all_scripts[i].editing_script;

            if(selected)
                name += " (X)";

            /*auto old_col = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);

            if(selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, old_col);
            }*/

            if(ImGui::MenuItem(name.c_str(), nullptr, selected, true))
            {
                if(i != current_idx)
                {
                    switch_to(i);
                }
            }

            /*if(selected)
            {
                ImGui::PopStyleColor(1);
            }*/

            if(ImGui::IsItemClicked(2) || ImGui::IsItemClicked(1))
            {
                close(i);
                i--;
                continue;
            }
        }


        ImGui::EndMenuBar();
    }

    /*for(int i=0; i < (int)all_scripts.size(); i++)
    {
        if(ImGui::Button(all_scripts[i].editing_script.c_str()))
        {

        }

        if(i != (int)all_scripts.size() - 1)
        {
            ImGui::SameLine();
        }
    }*/

    //current_script.has_unsaved_changes |= editor.IsTextChanged();

    if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        all_scripts[current_idx].has_unsaved_changes |= editor.IsTextChanged();
    }

    ImGui::PushFont(font_select.get_editor_font());

    editor.Render("TextRenderer");

    ImGui::PopFont();

    ImGui::End();
}

void text_editor_manager::tick()
{
    //current_script.tick();

    if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        all_scripts[current_idx].tick();
    }

    if(!is_open)
        return;

    if(!any_selected)
        return;

    if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        all_scripts[current_idx].set_contents(editor.GetText());
    }

    //current_script.set_contents(editor.GetText());
}
