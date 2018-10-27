#include "text_editor.hpp"
#include <libncclient/nc_util.hpp>
#include "local_commands.hpp"
#include "font_cfg.hpp"
#include <iostream>
#include <json/json.hpp>
#include <libncclient/nc_string_interop.hpp>
#include "tag_manager.hpp"

void editable_script::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(editing_script, ser);

    if(ser == false)
    {
        has_script = false;
        set_file_name(editing_script);

        editor.SetText(get_contents());
    }

    /*if(ser == true && has_script)
    {
        save();
    }*/
}

void user_scripts::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(user, ser);
    s.handle_serialise(current_idx, ser);
    s.handle_serialise(all_scripts, ser);
}

void text_editor_manager::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(selected_user, ser);
    s.handle_serialise(is_open, ser);
    s.handle_serialise(any_selected, ser);
    s.handle_serialise(force_save, ser);
    s.handle_serialise(modified, ser);
    s.handle_serialise(current_font_size, ser);
    s.handle_serialise(dirty_font, ser);
    s.handle_serialise(all_scripts, ser);
    s.handle_serialise(should_auto_indent, ser);

    if(ser == false)
    {
        dirty_font = true;
    }

    /*if(ser == false && selected_user != "")
    {
        user_scripts& current = all_scripts[selected_user];

        if(current.current_idx >= 0 && current.current_idx < (int)current.all_scripts.size())
        {
            editor.SetText(current.all_scripts[current.current_idx].get_contents());
        }
        else
        {
            editor.SetText(" ");
        }
    }

    if(ser == false && selected_user == "")
    {
        editor.SetText(" ");
    }*/
}

void editable_script::set_file_name(const std::string& file_name)
{
    ///resetting
    if(has_script)
        save();

    editing_script = file_name;
    friendly_name = format_raw_script_name(editing_script);

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

    //save_clock.restart();

    force_save = false;
    has_unsaved_changes = false;

    disk_contents_on_load = script_contents;
}

void editable_script::load()
{
    script_contents = read_file_bin("./scripts/" + editing_script);
    disk_contents_on_load = script_contents;
}

void editable_script::tick()
{
    if(!has_script)
        return;

    ///ONLY DO THIS ON MODIFICATIONS
    ///so we can detect if it were externally modified
    if(force_save)
    {
        save();
    }
}

void editable_script::change_name(const std::string& to)
{
    if(file_exists("./scripts/" + to))
        return;

    std::string base_name = "./scripts/" + editing_script;
    std::string new_name = "./scripts/" + to;

    editing_script = to;

    rename(base_name.c_str(), new_name.c_str());

    friendly_name = format_raw_script_name(editing_script);
}

std::string editable_script::get_contents()
{
    if(script_contents.size() == 0)
        return " ";

    return script_contents;
}

std::string editable_script::get_disk_contents()
{
    return read_file_bin("./scripts/" + editing_script);
}

std::string editable_script::get_contents_as_were_loaded()
{
    return disk_contents_on_load;
}

void editable_script::set_contents(const std::string& new_contents)
{
    script_contents = new_contents;
}

void editable_script::upload(c_shared_data data, bool tagged)
{
    ///#up_es6 name data

    std::string script_data = get_contents();
    std::string disk_name = editing_script;

    auto all = no_ss_split(disk_name, ".");

    if(all.size() < 2)
        return;

    tag_manager& tag_manage = get_global_tag_manager();

    std::string name = all[1];

    if(tag_manage.received_tag(name))
    {
        tag_manage.remove_tag(name);
    }

    std::string comm;

    if(tagged)
        comm = "client_command_tagged " + name + " #up_es6";
    else
        comm = "client_command #up_es6";

    std::string final_command = comm + " " + name + " " + script_data;

    sd_add_back_write(data, make_view(final_command));
}

void editable_script::run(c_shared_data data)
{
    std::string disk_name = editing_script;

    auto all = no_ss_split(disk_name, ".");

    if(all.size() < 2)
        return;

    std::string command = std::string("client_command ") + "#ns." + all[0] + "." + all[1] + "({})";

    sd_add_back_write(data, make_view(command));
}

void editable_script::schedule_run_after_upload(text_editor_manager& text_editor_manage)
{
    std::string disk_name = editing_script;

    auto all = no_ss_split(disk_name, ".");

    if(all.size() < 2)
        return;

    text_editor_manage.scheduled_runs.push_back({all[1], "client_command #ns." + all[0] + "." + all[1] + "({})"});
}

editable_script::editable_script()
{
    TextEditor::LanguageDefinition lang = TextEditor::LanguageDefinition::JavaScript();
    editor.SetLanguageDefinition(lang);

    TextEditor::Palette palette = editor.GetPalette();
    palette[(int)TextEditor::PaletteIndex::Identifier] = 0xFFFFFFFF;
    editor.SetPalette(palette);
}

editable_script::~editable_script()
{
    if(!has_script)
        return;

    if(has_unsaved_changes)
        save();
}

text_editor_manager::text_editor_manager(font_selector& _font_select) : font_select(_font_select)
{

}

text_editor_manager::~text_editor_manager()
{
    save_only_modified();
    //save(true);
}

void text_editor_manager::set_is_srgb(bool srgb)
{
    is_srgb = srgb;
}

void text_editor_manager::set_current_user(const std::string& username)
{
    editing_user = username;
}

void user_scripts::switch_to(int idx)
{
    /*if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        all_scripts[current_idx].save();
    }*/

    current_idx = idx;

    if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        all_scripts[current_idx].editor.SetText(all_scripts[current_idx].get_contents());
    }
}

void user_scripts::switch_to(const std::string& name)
{
    for(int i=0; i < (int)all_scripts.size(); i++)
    {
        if(all_scripts[i].editing_script == name)
        {
            switch_to(i);
        }
    }
}

void user_scripts::close(int idx)
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

        switch_to( tidx);
    }
}

void user_scripts::close(const std::string& name)
{
    for(int i=0; i < (int)all_scripts.size(); i++)
    {
        if(all_scripts[i].editing_script == name)
        {
            close(i);
            i--;
            continue;
        }
    }
}

void user_scripts::save()
{
    for(auto& i : all_scripts)
    {
        i.save();
    }
}

void user_scripts::save_editing()
{
    if(current_idx >= 0 && current_idx < (int)all_scripts.size())
    {
        all_scripts[current_idx].save();
    }
}

std::optional<editable_script*> user_scripts::get_current_script()
{
    if(current_idx < 0 || current_idx >= (int)all_scripts.size())
        return std::nullopt;

    return &all_scripts[current_idx];
}

/*void text_editor_manager::schedule_close(const std::string& name)
{
    to_close.push_back(name);
}*/

void text_editor_manager::render(c_shared_data data)
{
    //ImGui::PushFont(font_select.get_editor_font());

    if(!is_open)
        return;

    ImGui::Begin("Text Editor", &is_open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_ResizeFromAnySide);

    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        sf::Keyboard key;

        if(key.isKeyPressed(sf::Keyboard::LControl) && ONCE_MACRO(sf::Keyboard::S))
        {
            auto curr = all_scripts[selected_user].get_current_script();

            if(curr.has_value())
            {
                curr.value()->save();
            }
        }

        if(should_undo)
        {
            auto curr = all_scripts[selected_user].get_current_script();

            if(curr.has_value())
            {
                editable_script* edit = curr.value();

                edit->editor.Undo();
                edit->set_contents(edit->editor.GetText());
            }

            should_undo = false;
        }

        if(should_redo)
        {
            auto curr = all_scripts[selected_user].get_current_script();

            if(curr.has_value())
            {
                editable_script* edit = curr.value();

                edit->editor.Redo();
                edit->set_contents(edit->editor.GetText());
            }

            should_redo = false;
        }

        is_focused = true;
    }

    std::vector<std::string> script_names = get_all_scripts_list();
    //std::vector<std::string> script_names = get_scripts_list(editing_user);

    std::map<std::string, std::vector<std::string>> current_scripts;

    for(auto& i : script_names)
    {
        auto split = no_ss_split(i, ".");

        std::string usr_name = split[0];

        //std::string user_script = format_raw_script_name(i);

        current_scripts[usr_name].push_back(i);
    }

    if(current_scripts.find(editing_user) == current_scripts.end())
    {
        current_scripts[editing_user];
    }

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::BeginMenu("Scripts"))
            {
                for(auto& item : current_scripts)
                {
                    std::string user_name = item.first;

                    if(user_name == selected_user)
                        user_name = ">" + user_name;
                    else
                        user_name = " " + user_name;

                    if(ImGui::BeginMenu(user_name.c_str()))
                    {
                        if(ImGui::MenuItem("New Script..."))
                        {
                            editable_script script;

                            std::string partial_name = item.first + ".new_script";
                            std::string expanded_partial_name = partial_name;

                            int partial_id = 1;

                            while(file_exists("./scripts/" + expanded_partial_name + ".js"))
                            {
                                expanded_partial_name = partial_name + std::to_string(partial_id++);
                            }

                            script.set_file_name(expanded_partial_name + ".js");
                            script.save();

                            all_scripts[item.first].all_scripts.push_back(script);

                            all_scripts[item.first].switch_to((int)all_scripts[item.first].all_scripts.size() - 1);

                            selected_user = item.first;

                            any_selected = true;
                        }

                        for(auto& name : item.second)
                        {
                            std::string friendly_name = format_raw_script_name(name);

                            if(ImGui::MenuItem(friendly_name.c_str()))
                            {
                                editable_script script;
                                script.set_file_name(name);
                                //all_scripts.push_back(script);

                                all_scripts[item.first].all_scripts.push_back(script);

                                all_scripts[item.first].switch_to((int)all_scripts[item.first].all_scripts.size() - 1);

                                selected_user = item.first;

                                any_selected = true;
                            }
                        }

                        ImGui::EndMenu();
                    }
                }

                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu("Settings"))
            {
                ///font size
                {
                    if(ImGui::Button("-"))
                    {
                        current_font_size -= 0.5;
                        dirty_font = true;
                    }

                    ImGui::SameLine();

                    std::string str = std::to_string(current_font_size);

                    ImGui::Text(str.c_str());

                    ImGui::SameLine();

                    if(ImGui::Button("+"))
                    {
                        current_font_size += 0.5;
                        dirty_font = true;
                    }
                }

                ///auto indent
                {
                    if(ImGui::Checkbox("Auto Indent?", &should_auto_indent))
                    {
                        update_auto_indent();
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        for(auto& i : current_scripts)
        {
            std::string user_name = i.first;

            if(user_name == selected_user)
                user_name = ">" + user_name;
            else
                user_name = " " + user_name;

            if(ImGui::MenuItem(user_name.c_str()))
            {
                selected_user = i.first;

                all_scripts[selected_user].switch_to(all_scripts[selected_user].current_idx);
            }
        }

        ImGui::EndMenuBar();
    }


    ImGui::BeginChild("ScriptBar", ImVec2(0,0), false, ImGuiWindowFlags_MenuBar);

    ImGui::BeginMenuBar();

    {
        user_scripts& currently_editing = all_scripts[selected_user];

        for(int i=0; i < (int)currently_editing.all_scripts.size(); i++)
        {
            editable_script& current_script = currently_editing.all_scripts[i];

            bool selected = i == currently_editing.current_idx;

            std::string name = current_script.editing_script;

            std::string friendly_name = current_script.friendly_name;

            if(selected)
                friendly_name = ">" + friendly_name + " ";
            else
                friendly_name = " " + friendly_name + " ";

            if(current_script.has_unsaved_changes)
            {
                friendly_name.back() = '*';
            }

            if(ImGui::MenuItem(friendly_name.c_str(), nullptr, selected, true))
            {
                if(i != currently_editing.current_idx)
                {
                    currently_editing.switch_to( name);
                }
            }

            if(current_script.is_renaming)
            {
                ImGui::SetNextWindowFocus();
                ImGui::Begin("Rename", &current_script.is_renaming, ImGuiWindowFlags_AlwaysAutoResize);

                char input[100] = {0};

                for(int i=0; i < 99 && i < (int)name.size(); i++)
                {
                    input[i] = name[i];
                }

                if(ImGui::InputText("", &input[0], 99, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    std::string str(std::begin(input), std::begin(input) + strlen(input));

                    bool valid = true;

                    for(int i=0; i < (int)str.size(); i++)
                    {
                        if(str[i] == ' ')
                            valid = false;
                    }

                    if(valid)
                    {
                        current_script.is_renaming = false;
                        current_script.change_name(str);
                    }
                }

                ImGui::End();
            }

            if(ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
            {
                current_script.is_renaming = true;
            }

            if(ImGui::IsItemClicked(2) || ImGui::IsItemClicked(1))
            {
                currently_editing.close(i);
                break;
            }

            if(ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Double click to rename\nRight click to close");
            }
        }
    }

    ImGui::EndMenuBar();

    auto opt = all_scripts[selected_user].get_current_script();

    if(opt.has_value())
    {
        ImGui::BeginChild("NewChild", ImVec2(0,0), false, ImGuiWindowFlags_MenuBar);

        ImGui::BeginMenuBar();

        {
            if(ImGui::MenuItem("Upload"))
            {
                opt.value()->upload(data);
            }

            if(ImGui::MenuItem("Run"))
            {
                opt.value()->run(data);
            }

            if(ImGui::MenuItem("Upload+Run"))
            {
                opt.value()->upload(data, true);
                opt.value()->schedule_run_after_upload(*this);
            }
        }

        ImGui::EndMenuBar();
    }


    /*for(auto& i : all_scripts)
    {
        int current_idx = i.second.current_idx;

        if(current_idx >= 0 && current_idx < (int)i.second.all_scripts.size())
        {
            i.second.all_scripts[current_idx].has_unsaved_changes |= editor.IsTextChanged();
        }
    }*/

    ImGui::PushFont(font_select.get_editor_font());

    //editor.Render("TextRenderer");

    if(selected_user != "")
    {
        auto found = all_scripts[selected_user].get_current_script();

        if(found.has_value())
        {
            editable_script* scr = *found;

            scr->editor.Render("TextRenderer");
        }
    }

    ImGui::PopFont();

    if(opt)
        ImGui::EndChild();

    ImGui::EndChild();

    //ImGui::PopStyleVar(1);

    ImGui::End();

    tag_manager& tag_manage = get_global_tag_manager();

    for(int i=0; i < (int)scheduled_runs.size(); i++)
    {
        server_tagged_message& tagged = scheduled_runs[i];

        if(tag_manage.received_tag(tagged.tag))
        {
            tag_manage.remove_tag(tagged.tag);

            sd_add_back_write(data, make_view(tagged.message));

            scheduled_runs.erase(scheduled_runs.begin() + i);
            i--;
            continue;
        }
    }
}

void text_editor_manager::tick()
{
    //current_script.tick();

    for(auto& i : all_scripts)
    {
        int current_idx = i.second.current_idx;

        if(current_idx >= 0 && current_idx < (int)i.second.all_scripts.size())
        {
            i.second.all_scripts[current_idx].tick();
        }
    }

    if(!is_open)
        return;

    /*if(!any_selected)
        return;*/

    int current_idx = all_scripts[selected_user].current_idx;

    if(current_idx >= 0 && current_idx < (int)all_scripts[selected_user].all_scripts.size())
    {
        //std::cout << "contents " << editor.GetText() << std::endl;

        editable_script& editable =all_scripts[selected_user].all_scripts[current_idx];

        std::string current_contents = editable.get_contents();
        std::string editor_text = editable.editor.GetText();

        if(current_contents != editor_text)
        {
            editable.has_unsaved_changes = true;
        }

        editable.set_contents(editable.editor.GetText());
    }

    for(auto& i : all_scripts)
    {
        for(auto& script : i.second.all_scripts)
        {
            if(script.editor.IsSrgb() != is_srgb)
                script.editor.SetIsSrgb(is_srgb);
        }
    }
}

void text_editor_manager::on_focus_window()
{
    should_check_for_modifications = true;
}

void text_editor_manager::check_for_external_modifications()
{
    if(should_check_for_modifications)
    {
        for(auto& i : all_scripts)
        {
            user_scripts& scripts = i.second;

            for(editable_script& script : scripts.all_scripts)
            {
                std::string disk = script.get_disk_contents();
                std::string original_disk = script.get_contents_as_were_loaded();

                if(disk != original_disk)
                {
                    modified_scripts.insert(script.editing_script);

                    display_modifications_window = true;
                }
            }
        }
    }

    if(modified_scripts.size() == 0)
    {
        display_modifications_window = false;
    }

    if(display_modifications_window)
    {
        ImGui::SetNextWindowFocus();
        ImGui::Begin("Files were modified externally", &display_modifications_window, ImGuiWindowFlags_AlwaysAutoResize);

        for(auto it = modified_scripts.begin(); it != modified_scripts.end();)
        {
            bool any_erased = false;

            std::string full_name = *it;

            ImGui::Text(full_name.c_str());

            ImGui::SameLine();

            ///remember to refresh editor here
            if(ImGui::Button("Keep Disk"))
            {
                auto found = name_to_editable(full_name);

                ///need to reset editor here if were're currently editing
                if(found.has_value())
                {
                    editable_script* edit = found.value();

                    edit->load();

                    auto f2 = all_scripts[selected_user].get_current_script();

                    if(f2.has_value())
                    {
                        editable_script* script = *f2;

                        if(script->editing_script == full_name)
                        {
                            edit->editor.SetText(edit->get_contents());
                        }
                    }
                }

                any_erased = true;
                it = modified_scripts.erase(it);
            }

            ImGui::SameLine();

            if(ImGui::Button("Keep Current"))
            {
                auto found = name_to_editable(full_name);

                if(found.has_value())
                {
                    editable_script* edit = found.value();

                    edit->save();
                }

                any_erased = true;
                it = modified_scripts.erase(it);
            }

            if(!any_erased)
                it++;
        }

        ImGui::End();
    }

    should_check_for_modifications = false;
}

void text_editor_manager::save(bool full)
{
    std::string settings_file = "text_sett.txt";

    serialise stem;
    stem.handle_serialise_no_clear(*this, true);
    stem.save(settings_file);

    if(full)
    {
        for(auto& i : all_scripts)
        {
            for(auto& j : i.second.all_scripts)
            {
                j.save();
            }
        }
    }
    else if(selected_user != "")
    {
        all_scripts[selected_user].save_editing();
    }
}

void text_editor_manager::save_only_modified()
{
    save_settings();

    for(auto& i : all_scripts)
    {
        for(auto& j : i.second.all_scripts)
        {
            if(j.has_unsaved_changes)
                j.save();
        }
    }
}

void text_editor_manager::save_settings()
{
    std::string settings_file = "text_sett.txt";

    serialise stem;
    stem.handle_serialise_no_clear(*this, true);
    stem.save(settings_file);
}

void text_editor_manager::load()
{
    std::string settings_file = "text_sett.txt";

    if(file_exists(settings_file))
    {
        serialise ssett;
        ssett.load(settings_file);
        ssett.handle_serialise_no_clear(*this, false);
    }

    update_auto_indent();
}

void text_editor_manager::update_auto_indent()
{
    for(auto& i : all_scripts)
    {
        for(auto& k : i.second.all_scripts)
        {
            k.editor.SetShouldAutoIndent(should_auto_indent);
        }
    }
}

std::optional<editable_script*> text_editor_manager::name_to_editable(const std::string& editable_name)
{
    for(auto& i : all_scripts)
    {
        for(auto& j : i.second.all_scripts)
        {
            if(j.editing_script == editable_name)
                return &j;
        }
    }

    return {};
}
