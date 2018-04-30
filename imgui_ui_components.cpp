#include "imgui_ui_components.hpp"
#include <imgui/imgui.h>
#include "string_helpers.hpp"
#include <libncclient/nc_util.hpp>
#include <libncclient/nc_string_interop.hpp>
#include <libncclient/c_server_api.h>
#include "ui_components.hpp"
#include "copy_handler.hpp"

void scrollbar_hack::do_hack(int approx_num, bool set_scrollbar)
{
    /*float base_y = win.getSize().y;

    float my_y = base_y - scrolled * char_inf::cheight;

    float max_y = base_y - approx_num * char_inf::cheight;

    if(max_y < -char_inf::cheight)
    {
        std::cout << "hack at " << max_y << std::endl;

        //ImGui::SetCursorScreenPos(ImVec2(0.f, max_y));
        ImGui::SetCursorPos(ImVec2(0, max_y));
        ImGui::Text("Invis");
    }*/

    ImGui::BeginChild("right_child", ImVec2(0,0), false, ImGuiWindowFlags_NoScrollWithMouse);

    for(int i=0; i < approx_num; i++)
    {
        ImGui::Text("\n");
    }

    if(set_scrollbar)
    {
        ///scrolled is lines above 0
        float scrolled_frac = scrolled / approx_num;
        float ivscrolled = 1.f - scrolled_frac;

        ImGui::SetScrollY(ivscrolled * ImGui::GetScrollMaxY());
    }

    output_scroll_frac = ImGui::GetScrollY() / ImGui::GetScrollMaxY();

    if(ImGui::IsMouseDown(0) && ImGui::IsWindowFocused())
    {
        scrolling = true;
    }
    else
    {
        scrolling = false;
    }

    ImGui::EndChild();
}

void terminal_imgui::do_serialise(serialise& s, bool ser)
{
    if(ser == false)
    {
        auto_handle.found_unprocessed_autocompletes.clear();
        auto_handle.found_args.clear();
        auto_handle.is_valid.clear();
    }

    s.handle_serialise(text_history, ser);
    s.handle_serialise(chat_threads, ser);
    s.handle_serialise(command, ser);
    s.handle_serialise_no_clear(auto_handle, ser);

    //std::cout << "loaded hist " << text_history.size() << std::endl;
}

void terminal_imgui::clear_terminal()
{
    text_history.clear();
}

void terminal_imgui::clear_chat()
{
    chat_threads.clear();
}

void terminal_imgui::clear_text()
{
    clear_terminal();
    clear_chat();
}

terminal_imgui::terminal_imgui()
{
    auto_handle.use_autocomplete = true;
    auto_handle.use_autocolour = true;
}

enum render_instruction
{
    imgui_text,
    newline,
};

struct render_command
{
    render_instruction type = imgui_text;
    std::string str;
    vec3f col;
    vec2f absolute_pos;
};

void render_copy_aware(vec3f col, const std::string& str, vec2f start_pos, vec2f end_pos, vec2f render_pos)
{
    int cutoff_point = -1;

    std::vector<std::pair<std::string, vec3f>> cols;

    std::pair<std::string, vec3f> cur_str;

    if(str.size() == 0)
        return;

    copy_handler* handle = get_global_copy_handler();
    bool is_inside = handle->char_is_within_select_box(start_pos);

    for(int i=0; i < (int)str.size(); i++)
    {
        float ffrac = (float)i / (float)str.size();

        vec2f pos = mix(start_pos, end_pos, ffrac);

        bool currently_inside = handle->char_is_within_select_box(pos);

        if(currently_inside != is_inside)
        {
            cols.push_back(cur_str);
            cur_str = decltype(cur_str)();
            is_inside = currently_inside;
        }

        cur_str.first.push_back(str[i]);

        if(currently_inside)
        {
            cur_str.second = {80, 80, 255};

            if(cur_str.first.back() == ' ')
                cur_str.first.back() = '-';
        }
        else
            cur_str.second = col;
    }

    if(cur_str.first.size() > 0)
    {
        cols.push_back(cur_str);
    }

    vec2f c_pos = render_pos;

    for(int i=0; i < (int)cols.size(); i++)
    {
        vec3f ccol = cols[i].second;
        const std::string& cstr = cols[i].first;

        ImGui::SetCursorScreenPos(ImVec2(c_pos.x(), c_pos.y()));

        ImGui::TextColored(ImVec4(ccol.x()/255.f, ccol.y()/255.f, ccol.z()/255.f, 1.f), cstr.c_str());

        c_pos.x() += cstr.size() * char_inf::cwidth;

        if(i != (int)cols.size() - 1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

void render_copy_blind(vec3f col, const std::string& str, vec2f render_pos)
{
    ImGui::SetCursorScreenPos(ImVec2(render_pos.x(), render_pos.y()));

    ImGui::TextColored(ImVec4(col.x()/255.f, col.y()/255.f, col.z()/255.f, 1.f), str.c_str());
}

void imgui_render_str(const std::vector<formatted_char>& text, std::vector<std::vector<formatted_char>>& formatted_text, float window_width)
{
    copy_handler* handle = get_global_copy_handler();

    std::vector<render_command> commands;

    render_command current;

    bool restart = true;

    for(const formatted_char& fchar : text)
    {
        if(restart)
        {
            current.absolute_pos = fchar.render_pos;
            restart = false;
        }

        if(fchar.ioc.col != current.col || fchar.ioc.c == '\n' || fchar.render_pos.y() != current.absolute_pos.y())
        {
            restart = true;

            if(current.str.size() > 0 && fchar.ioc.c != '\n')
            {
                current.type = imgui_text;
                commands.push_back(current);
                current = render_command();
            }

            if(fchar.ioc.c == '\n' || fchar.render_pos.y() != current.absolute_pos.y())
            {
                if(current.str.size() > 0)
                {
                    current.type = imgui_text;
                    commands.push_back(current);
                    current = render_command();
                }

                render_command next;
                next.type = newline;
                commands.push_back(next);
                //continue;
            }
        }

        if(restart)
        {
            current.absolute_pos = fchar.render_pos;
            restart = false;
        }

        current.col = fchar.ioc.col;
        current.str += fchar.ioc.c;
    }

    if(current.str.size() > 0)
    {
        commands.push_back(current);
    }

    if(commands.size() == 0)
    {
        render_command next;
        next.type = newline;
        commands.push_back(next);
    }

    formatted_text.emplace_back();

    std::vector<formatted_char>& chars = formatted_text.back();

    for(int kk=0; kk < (int)commands.size(); kk++)
    {
        render_command& next = commands[kk];

        if(next.type == newline)
        {
            //ImGui::Text("\n");
            continue;
        }

        vec2f pos = next.absolute_pos;
        std::string str = next.str;
        vec3f col = next.col;

        //auto spos = ImGui::GetCursorScreenPos();

        auto spos = pos;

        ///need to predict here if the text is hilighted or not
        ///then if it is, replace spaces with "-" and colour blue
        float width = ImGui::CalcTextSize(str.c_str(), nullptr, false, window_width).x;

        if(handle->held)
            render_copy_aware(col, str, spos, (vec2f){spos.x(), spos.y()} + (vec2f){width, 0.f}, pos);
        else
            render_copy_blind(col, str, pos);

        //std::cout << "pos " << pos << std::endl;

        float x_start = spos.x();
        float x_end = spos.x() + width;

        float y_coord = spos.y();

        for(int ccount = 0; ccount < (int)str.size(); ccount++)
        {
            float ffrac = (float)ccount / (float)str.size();

            formatted_char chr;
            chr.ioc.c = str[ccount];

            chr.render_pos.y() = y_coord;
            chr.render_pos.x() = ffrac * x_end + (1.f - ffrac) * x_start;

            chars.push_back(chr);
        }

        /*if(kk == (int)commands.size()-1)
            ImGui::NewLine();*/

        if(kk != (int)commands.size()-1)
            ImGui::SameLine(0, char_inf::extra_glyph_spacing);
    }
}

void render_handle_imgui(scrollbar_hack& scroll_hack, std::string& command, int& cursor_pos_idx, const std::vector<interop_vec_t>& text_history, auto_handler& auto_handle, std::vector<std::vector<formatted_char>>& formatted_text, float extra_shrink = 0)
{
    float overall_width = ImGui::GetWindowWidth();

    ImGui::BeginChild("left_sub", ImVec2(overall_width - 40 - extra_shrink, 0.f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    bool set_scroll = false;

    if(ImGui::IsWindowHovered() && scroll_hack.scrolled_this_frame != 0)
    {
        scroll_hack.scrolled += scroll_hack.scrolled_this_frame;
        scroll_hack.scrolled_this_frame = 0.f;
        set_scroll = true;
    }

    auto wrap_dim = ImGui::GetWindowSize();
    auto start = ImGui::GetWindowPos();

    wrap_dim.x += start.x;
    wrap_dim.y += start.y;

    vec2f current = {start.x, start.y};

    int vertical_columns = ceil((float)wrap_dim.y / char_inf::cheight);

    int min_start = (int)text_history.size() - vertical_columns;

    min_start = min_start - scroll_hack.scrolled;

    if(min_start < 0)
        min_start = 0;

    std::vector<std::vector<formatted_char>> formatted;

    auto all_interop = text_history;

    std::string render_command = command;
    bool specials = true;

    if(render_command == "")
    {
        render_command = "`bType something here...`";
        specials = false;
    }

    auto icommand = string_to_interop(render_command, specials, auto_handle, false);

    int cursor_offset = 0;

    auto_handle.handle_autocompletes(icommand, cursor_pos_idx, cursor_offset, command);

    interop_char curs;
    curs.col = {255, 255, 255};
    curs.c = '|';
    curs.is_cursor = true;

    int curs_cur = cursor_pos_idx + cursor_offset;

    if(ImGui::IsWindowFocused())
    {
        if(curs_cur >= (int)icommand.size())
            icommand.push_back(curs);
        else if(curs_cur >= 0 && curs_cur < (int)icommand.size())
            icommand.insert(icommand.begin() + curs_cur, curs);
    }

    all_interop.push_back(icommand);

    for(int i=min_start; i < (int)all_interop.size(); i++)
    {
        formatted.push_back(format_characters(all_interop[i], current, {start.x, start.y}, (vec2f){wrap_dim.x, wrap_dim.y}, 0.f));
    }

    internally_format(formatted, {start.x, start.y + ImGui::GetWindowHeight()}, scroll_hack.scrolled * char_inf::cheight);

    for(auto& i : formatted)
    {
        imgui_render_str(i, formatted_text, ImGui::GetWindowWidth());
    }

    ImGui::EndChild();

    ImGui::SameLine(0.f, 0.f);

    ///rough
    scroll_hack.do_hack(all_interop.size() + 1, true);

    if(scroll_hack.scrolling)
    {
        scroll_hack.scrolled = (1.f - scroll_hack.output_scroll_frac) * (all_interop.size() + 1.f);
    }
}

void terminal_imgui::render(sf::RenderWindow& win)
{
    copy_handler* handle = get_global_copy_handler();

    std::vector<std::vector<formatted_char>> formatted_text;

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(win.getSize().x, win.getSize().y));

    ImGui::Begin("asdf1", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    render_handle_imgui(scroll_hack, command.command, command.cursor_pos_idx, text_history, auto_handle, formatted_text);

    ImGui::End();

    handle->process_formatted(formatted_text);
}

#define MAX_TEXT_HISTORY 250

void terminal_imgui::bump_command_to_history()
{
    text_history.push_back(string_to_interop(command.command, true, auto_handle));
    command.clear_command();

    limit_size(text_history, MAX_TEXT_HISTORY);
    de_newline(text_history);
}

void terminal_imgui::add_text_from_server(const std::string& in, chat_window& chat_win, bool server_command)
{
    if(in == "")
        return;

    std::string str = in;

    server_command_info command_info = sa_server_response_to_info(make_view(in));

    bool push = false;

    if(server_command)
    {
        std::string command_str = "command ";
        std::string chat_api = "chat_api ";
        std::string scriptargs = "server_scriptargs ";
        std::string invalid_str = "server_scriptargs_invalid";
        std::string ratelimit_str = "server_scriptargs_ratelimit ";

        if(command_info.type == server_command_command)
        {
            str = c_str_consume(sa_command_to_human_readable(command_info));

            push = true;
        }
        else if(command_info.type == server_command_chat_api)
        {
            std::vector<std::string> chnls;
            std::vector<std::string> msgs;

            std::vector<std::string> in_channels;

            chat_api_info chat_info = sa_chat_api_to_info(command_info);

            for(int i=0; i < chat_info.num_msgs; i++)
            {
                chnls.push_back(c_str_sized_to_cpp(chat_info.msgs[i].channel));
                msgs.push_back(c_str_sized_to_cpp(chat_info.msgs[i].msg));
            }

            for(int i=0; i < chat_info.num_in_channels; i++)
            {
                in_channels.push_back(c_str_sized_to_cpp(chat_info.in_channels[i].channel));
            }

            sa_destroy_chat_api_info(chat_info);

            chat_win.set_side_channels(in_channels);

            for(int i=0; i < (int)chnls.size(); i++)
            {
                text_history.push_back(string_to_interop(msgs[i] + "\n", false, chat_win.auto_handle));

                chat_threads[chnls[i]].chats.push_back(string_to_interop(msgs[i], false, chat_win.auto_handle));
            }

            limit_size(text_history, MAX_TEXT_HISTORY);
            de_newline(text_history);
        }
        else if(command_info.type == server_command_server_scriptargs)
        {
            std::cout << str << std::endl;

            script_argument_list args = sa_server_scriptargs_to_list(command_info);

            if(args.scriptname.str != nullptr && args.scriptname.num > 0)
            {
                std::vector<autocomplete_args> auto_args;

                for(int i=0; i < args.num; i++)
                {
                    std::string key = c_str_sized_to_cpp(args.args[i].key);
                    std::string val = c_str_sized_to_cpp(args.args[i].val);

                    auto_args.push_back({key, val});
                }

                std::string scriptname = c_str_sized_to_cpp(args.scriptname);

                auto_handle.found_args[scriptname] = auto_args;
                auto_handle.is_valid[scriptname] = true;
            }

            sa_destroy_script_argument_list(args);
        }
        else if(command_info.type == server_command_server_scriptargs_invalid)
        {
            std::cout << "inv " << str << std::endl;

            std::string name = c_str_consume(sa_server_scriptargs_invalid_to_script_name(command_info));

            if(name.size() > 0)
            {
                auto_handle.is_valid[name] = false;
            }
        }
        else if(command_info.type == server_command_server_scriptargs_ratelimit)
        {
            std::string name = c_str_consume(sa_server_scriptargs_ratelimit_to_script_name(command_info));

            std::cout << "rl name " << name << std::endl;

            if(name.size() > 0)
            {
                auto_handle.found_unprocessed_autocompletes.insert(name);
            }
        }
        else
        {
            push = true;
        }
    }
    else
    {
        push = true;
    }

    if(push)
    {
        text_history.push_back(string_to_interop(str, false, auto_handle));

        limit_size(text_history, MAX_TEXT_HISTORY);
        consider_resetting_scrollbar = true;

        de_newline(text_history);
    }

    sa_destroy_server_command_info(command_info);
}

void chat_thread::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(chats, ser);
}

void chat_window::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(render_start, ser);
    s.handle_serialise(side_buttons, ser);
    s.handle_serialise(selected, ser);
    s.handle_serialise(command, ser);
}

sf::Color chat_window::get_frame_col()
{
    return sf::Color(frame_col.x()*255.f, frame_col.y()*255.f, frame_col.z()*255.f, 255);
}

sf::Color chat_window::get_highlight_col()
{
    return sf::Color(70, 70, 70, 255);
}

void chat_window::tick()
{
    /*for(auto& i : side_buttons)
    {
        if(i.txt == selected)
        {
            i.is_selected = true;
        }
        else
        {
            i.is_selected = false;
        }
    }*/
}

void chat_window::render(sf::RenderWindow& win, std::map<std::string, chat_thread>& threads)
{
    std::vector<std::vector<formatted_char>> formatted;

    chat_thread& thread = threads[selected];

    if(!focus_once)
        ImGui::SetNextWindowFocus();

    focus_once = true;

    std::string chat_str = selected + "###chat_window";

    ImGui::Begin(chat_str.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ResizeFromAnySide);

    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    ImGui::BeginChild("left_selector", ImVec2(80, 0));

    for(auto& i : side_buttons)
    {
        if(ImGui::Button(i.c_str(), ImVec2(60, 0)))
        {
            selected = i;
        }
    }

    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    render_handle_imgui(scroll_hack, command.command, command.cursor_pos_idx, thread.chats, auto_handle, formatted, 80);

    ImGui::End();

    /*vec2f swidth = {win.getSize().x, win.getSize().y};

    vec2f render_pos = (vec2f) {swidth.x() - dim.x() - border_size, border_size};
    render_start = render_pos;

    sf::RectangleShape shape;

    shape.setSize({dim.x(), dim.y()});
    shape.setOutlineColor(get_frame_col());
    shape.setOutlineThickness(border_size);
    shape.setFillColor(sf::Color(30,30,30,255));

    shape.setPosition(render_pos.x(), render_pos.y());

    win.draw(shape);

    chat_thread& thread = threads[selected];

    ::render(win, command.command, thread.chats, command.cursor_pos_idx, {render_pos.x(), dim.y()}, {(int)win.getSize().x - char_inf::cwbuf/2.f - border_size, win.getSize().y}, border_size, auto_handle, focused);

    render_side_attachment(win);*/
}

#if 0
void chat_window::render_side_attachment(sf::RenderWindow& win)
{
    vec2f side_pos = {render_start.x() - side_dim.x() - border_size, render_start.y()};

    sf::RectangleShape shape;

    shape.setSize({side_dim.x(), side_dim.y()});
    shape.setOutlineColor(get_frame_col());
    shape.setOutlineThickness(border_size);
    shape.setFillColor(sf::Color(30, 30, 30, 255));

    shape.setPosition(side_pos.x(), side_pos.y());

    win.draw(shape);

    vec2f start_pos = {side_pos.x() + char_inf::cwbuf, side_pos.y() + char_inf::cheight/4.f};
    vec2f current_pos = start_pos;

    for(int i=0; i < (int)side_buttons.size(); i++)
    {
        side_buttons[i].pos = current_pos;
        side_buttons[i].dim = {side_dim.x(), char_inf::cheight};

        if(side_buttons[i].is_selected)
        {
            sf::RectangleShape shape;
            shape.setPosition({current_pos.x() - char_inf::cwbuf, current_pos.y()});
            shape.setFillColor(get_highlight_col());

            vec2f cdim = side_buttons[i].dim;
            //cdim.x() -= char_inf::cwbuf;

            shape.setSize({cdim.x(), cdim.y()});

            win.draw(shape);
        }

        auto ichars = string_to_interop_no_autos(side_buttons[i].txt, false);

        render_str(win, ichars, current_pos, start_pos, start_pos + side_dim, border_size);

        current_pos.y() += char_inf::cheight;
    }
}
#endif // 0

#if 0
void chat_window::process_click(vec2f pos)
{
    bool any = false;

    for(button& b : side_buttons)
    {
        if(b.within(pos))
        {
            any = true;
            break;
        }
    }

    if(!any)
        return;

    for(button& b : side_buttons)
    {
        if(b.within(pos))
        {
            selected = b.txt;
            b.is_selected = true;
        }
        else
        {
            b.is_selected = false;
        }
    }
}
#endif // 0

bool chat_window::within(vec2f pos)
{
    vec2f tl = render_start - side_dim;
    vec2f br = render_start + (vec2f) {dim.x(), dim.y()};

    return pos.x() >= tl.x() && pos.y() >= tl.y() &&
           pos.x() < br.x() && pos.y() < br.y();
}

void chat_window::set_side_channels(const std::vector<std::string>& sides)
{
    side_buttons.clear();

    for(auto& i : sides)
    {
        /*if(i == selected)
            side_buttons.push_back({i});
        else
            side_buttons.push_back({i});*/

        side_buttons.push_back(i);
    }
}
