#include "auth_manager.hpp"

#include <networking/networking.hpp>
#include <toolkit/fs_helpers.hpp>
#include "steam_api.hpp"
#include "util.hpp"
#include <imgui/imgui.h>
#include "string_helpers.hpp"
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "imgui_ui_components.hpp"
#include <networking/networking.hpp>
#include "render_string.hpp"

void auth_manager::check(steamapi& s_api, connection_send_data& to_write, const std::string& current_user)
{
    if(am_authenticated)
    {
        should_display_dialogue = false;
    }

    if(s_api.enabled)
    {
        ///embed hex_key.key
        if(file::exists("hey_key.key"))
        {
            printf("Embedding key auth in steam auth\n");

            s_api.request_auth_token(file::read("hex_key.key", file::mode::BINARY));
        }
        else
        {
            printf("Steam auth, no key auth");

            s_api.request_auth_token("");
        }

        while(s_api.should_wait_for_encrypted_token())
        {
            s_api.pump_callbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if(!s_api.auth_success())
        {
            printf("Failed to get steam token");
            return;
            //throw std::runtime_error("Could not fetch steam token");
        }

        auto res = s_api.get_encrypted_token();

        std::string etoken = std::string(res.begin(), res.end());

        nlohmann::json data;
        data["type"] = "steam_auth";
        data["data"] = binary_to_hex(etoken);

        write_data dat;
        dat.id = -1;
        dat.data = data.dump();

        to_write.write_to_websocket(std::move(dat));

        printf("Postwrite\n");
    }
    ///use key based auth
    else if(file::exists("hex_key.key"))
    {
        printf("Pure key auth\n");

        nlohmann::json data;
        data["type"] = "key_auth";
        data["data"] = file::read("hex_key.key", file::mode::BINARY);

        write_data dat;
        dat.id = -1;
        dat.data = data.dump();

        to_write.write_to_websocket(std::move(dat));
    }
    else
    {
        should_display_dialogue = true;
    }

    if(current_user.size() > 0)
    {
        nlohmann::json data;
        data["type"] = "generic_server_command";
        data["data"] = "user " + current_user;

        write_data dat;
        dat.id = -1;
        dat.data = data.dump();

        to_write.write_to_websocket(std::move(dat));
    }
}

void auth_manager::display(terminal2& term, steamapi& s_api, connection_send_data& to_write, const std::string& current_user)
{
    if(am_authenticated)
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x * 0.5f + viewport->Pos.x, viewport->Size.y * 0.5f + viewport->Pos.y), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowFocus();

    if(ImGui::Begin("No Auth Found", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("Drag and drop a valid *.key file into the window");

        ImGui::Text("Input hex_key.key");

        ImGui::SameLine();

        ImGui::PushItemWidth(16 * char_inf::cwidth + char_inf::cwidth * 2);

        ImGui::InputText(" ###inputtexto", &auth_dialogue_text);

        ImGui::PopItemWidth();

        //ImGui::SameLine();

        if(auth_dialogue_text.size() != 256)
        {

        }
        else
        {
            if(ImGui::Button("Enter"))
            {
                file::write("hex_key.key", auth_dialogue_text, file::mode::BINARY);
                check(s_api, to_write, current_user);

                if(should_display_dialogue)
                {
                    term.add_main_text("Something went wrong, try again");
                }
            }
        }

        if(ImGui::Button("Login as guest"))
        {
            nlohmann::json data;
            data["type"] = "generic_server_command";
            data["data"] = "#make_free_auth";

            write_data dat;
            dat.id = -1;
            dat.data = data.dump();

            to_write.write_to_websocket(std::move(dat));
        }
    }

    ImGui::End();
}

void auth_manager::extract_server_commands(terminal2& output, nlohmann::json& in)
{
    std::string type = in["type"];

    if(type == "server_msg")
    {
        if(in.count("authenticated") > 0 && in["authenticated"] == 1)
        {
            am_authenticated = true;

            printf("Authed!\n");
        }
    }
    else if(type == "auth")
    {
        std::string key = in["data"];
        std::string key_file = "hex_key.key";

        if(!file::exists(key_file))
        {
            file::write(key_file, key, file::mode::BINARY);

            output.add_main_text(make_success_col("Successfully downloaded auth to key.key"));
        }
        else
        {
            output.add_main_text(make_error_col("A key.key file already exists, please rename it first to avoid overwriting"));
        }
    }
}
