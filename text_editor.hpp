#ifndef TEXT_EDITOR_HPP_INCLUDED
#define TEXT_EDITOR_HPP_INCLUDED

#include <ImGuiColorTextEdit/TextEditor.h>

///we need the concept of a currently open script and tabbed scripts
///and also scripts need to never be able to have lost changes
struct text_editor_manager
{
    TextEditor editor;
    std::string editing_user;

    std::string editing_script;
    std::string script_contents;

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
                            editing_script = name;
                            script_contents = read_file_bin("./scripts/" + editing_script);

                            editor.SetText(script_contents);
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
        if(!is_open && !force_save)
            return;

        if(!any_selected && !force_save)
            return;

        script_contents = editor.GetText();

        ///ONLY DO THIS ON MODIFICATIONS
        ///so we can detect if it were externally modified
        if(save_clock.getElapsedTime().asSeconds() > 60 || force_save)
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
    }
};


#endif // TEXT_EDITOR_HPP_INCLUDED
