#ifndef TEXT_EDITOR_HPP_INCLUDED
#define TEXT_EDITOR_HPP_INCLUDED

#include <ImGuiColorTextEdit/TextEditor.h>

struct editable_script
{
    std::string editing_script;
    std::string script_contents;

    bool has_script = false;

    bool force_save = false;

    sf::Clock save_clock;

    void set_file_name(const std::string& file_name)
    {
        ///resetting
        if(has_script)
            save();

        editing_script = file_name;
        has_script = true;

        load();
    }

    void save()
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

    void load()
    {
        script_contents = read_file_bin("./scripts/" + editing_script);
    }

    void tick()
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

    std::string get_contents()
    {
        return script_contents;
    }

    void set_contents(const std::string& new_contents)
    {
        script_contents = new_contents;
    }

    ~editable_script()
    {
        if(!has_script)
            return;

        save();
    }
};

///we need the concept of a currently open script and tabbed scripts
///and also scripts need to never be able to have lost changes
struct text_editor_manager
{
    TextEditor editor;
    std::string editing_user;

    editable_script current_script;

    bool is_open = true;
    bool any_selected = false;
    bool force_save = false;
    bool modified = false;

    sf::Clock save_clock;

    text_editor_manager()
    {
        auto lang = TextEditor::LanguageDefinition::Lua();
        editor.SetLanguageDefinition(lang);
    }

    void set_current_user(const std::string& username)
    {
        editing_user = username;
    }

    void render()
    {
        ImGui::Begin("Text Editor", &is_open, ImGuiWindowFlags_MenuBar);

        if(ImGui::IsItemFocused())
        {
            sf::Keyboard key;

            if(key.isKeyPressed(sf::Keyboard::LControl) && ONCE_MACRO(sf::Keyboard::S))
            {
                force_save = true;
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
                            current_script.set_file_name(name);

                            editor.SetText(current_script.get_contents());
                            any_selected = true;
                        }
                    }

                    ImGui::EndMenu();
                }

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

        editor.Render("TextRenderer");

        ImGui::End();
    }

    void tick()
    {
        current_script.tick();

        if(!is_open)
            return;

        if(!any_selected)
            return;

        current_script.set_contents(editor.GetText());
    }
};


#endif // TEXT_EDITOR_HPP_INCLUDED
