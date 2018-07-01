#ifndef TEXT_EDITOR_HPP_INCLUDED
#define TEXT_EDITOR_HPP_INCLUDED

#include <ImGuiColorTextEdit/TextEditor.h>
#include <SFML/Graphics.hpp>
#include "util.hpp"
#include <serialise/serialise.hpp>

struct editable_script : serialisable
{
    std::string editing_script;
    std::string friendly_name;
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

    virtual void do_serialise(serialise& s, bool ser);

    ~editable_script();
};

struct user_scripts : serialisable
{
    std::string user;
    int current_idx = -1;

    std::vector<editable_script> all_scripts;

    void switch_to(TextEditor& editor, int idx);
    void switch_to(TextEditor& editor, const std::string& name);

    void close(TextEditor& editor, int idx);
    void close(TextEditor& editor, const std::string& name);

    virtual void do_serialise(serialise& s, bool ser);

    void save();
};

struct font_selector;

///we need the concept of a currently open script and tabbed scripts
///and also scripts need to never be able to have lost changes
struct text_editor_manager : serialisable
{
    TextEditor editor;
    std::string editing_user;
    std::string selected_user;
    //std::vector<std::string> to_close;

    sf::Clock settings_save_clock;

    //editable_script current_script;

    std::map<std::string, user_scripts> all_scripts;

    //std::vector<editable_script> all_scripts;
    //int current_idx = -1;

    bool is_open = true;
    bool any_selected = false;
    bool force_save = false;
    bool modified = false;

    ///need to save this
    ///and save the whole class generally
    float current_font_size = 14.f;
    bool dirty_font = false;

    font_selector& font_select;

    sf::Clock save_clock;

    text_editor_manager(font_selector& _font_select);
    ~text_editor_manager();

    void set_current_user(const std::string& username);

    //void schedule_close(const std::string& name);

    void render();

    void tick();

    virtual void do_serialise(serialise& s, bool ser);

    void save();
    void load();
};


#endif // TEXT_EDITOR_HPP_INCLUDED
