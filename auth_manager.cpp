#include "auth_manager.hpp"

#include <networking/networking.hpp>
#include <toolkit/fs_helpers.hpp>
#include "steam_api.hpp"
#include "util.hpp"
#include <imgui/imgui.h>
#include "string_helpers.hpp"
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "imgui_ui_components.hpp"

void auth_manager::check(steamapi& s_api, connection& conn, const std::string& current_user)
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
            printf("Failed to get encrypted token");
            throw std::runtime_error("Could not fetch steam token");
        }

        auto res = s_api.get_encrypted_token();

        std::string etoken = std::string(res.begin(), res.end());

        nlohmann::json data;
        data["type"] = "steam_auth";
        data["data"] = binary_to_hex(etoken);

        conn.write(data.dump());

        printf("Postwrite\n");
    }
    ///use key based auth
    else if(file::exists("hex_key.key"))
    {
        printf("Pure key auth\n");

        nlohmann::json data;
        data["type"] = "key_auth";
        data["data"] = file::read("hex_key.key", file::mode::BINARY);

        conn.write(data.dump());
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

        conn.write(data.dump());
    }
}

void auth_manager::display(terminal_imgui& term, steamapi& s_api, connection& conn, const std::string& current_user)
{
    if(am_authenticated)
        return;

    ImGui::Begin("No Auth Found", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("hex_key.key");

    ImGui::SameLine();

    ImGui::PushItemWidth(16 * char_inf::cwidth + char_inf::cwidth * 2);

    ImGui::InputText(" ###inputtexto", &auth_dialogue_text);

    ImGui::PopItemWidth();

    ImGui::SameLine();

    if(auth_dialogue_text.size() != 256)
    {
        ImGui::Text("Auth Hex is 256 characters long");
    }
    else
    {
        if(ImGui::Button("Enter"))
        {
            file::write("hex_key.key", auth_dialogue_text, file::mode::BINARY);
            check(s_api, conn, current_user);

            if(should_display_dialogue)
            {
                term.add_text("Something went wrong, try again");
            }
        }
    }

    ImGui::Text("Or drag and drop a valid *.key file into the window");

    ImGui::End();
}
