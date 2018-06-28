#ifndef TEXT_EDITOR_HPP_INCLUDED
#define TEXT_EDITOR_HPP_INCLUDED

#include <ImGuiColorTextEdit/TextEditor.h>
#include <SFML/Graphics.hpp>
#include "util.hpp"

struct editable_script
{
    std::string editing_script;
    std::string script_contents;

    bool has_script = false;
    bool force_save = false;
    bool has_unsaved_changes = false;

    sf::Clock save_clock;

    void set_file_name(const std::string& file_name);

    void save();
    void load();

    void tick();

    std::string get_contents();
    void set_contents(const std::string& new_contents);

    ~editable_script();
};

struct font_selector;

///we need the concept of a currently open script and tabbed scripts
///and also scripts need to never be able to have lost changes
struct text_editor_manager
{
    TextEditor editor;
    std::string editing_user;

    //editable_script current_script;

    std::vector<editable_script> all_scripts;
    int current_idx = -1;

    bool is_open = true;
    bool any_selected = false;
    bool force_save = false;
    bool modified = false;

    float current_font_size = 14.f;
    bool dirty_font = false;

    font_selector& font_select;

    sf::Clock save_clock;

    text_editor_manager(font_selector& _font_select);

    void set_current_user(const std::string& username);
    void switch_to(int idx);

    void close(int idx);

    void render();

    void tick();
};


#endif // TEXT_EDITOR_HPP_INCLUDED
