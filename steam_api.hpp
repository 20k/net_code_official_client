#ifndef STEAM_API_HPP_INCLUDED
#define STEAM_API_HPP_INCLUDED

#include <stdint.h>
#include <string>
#include <vector>

struct callback_environment;

struct steamapi
{
    bool enabled = false;
    bool overlay_open = false;

    steamapi();
    ~steamapi();

    void request_auth_token(const std::string& user_data);
    bool auth_success();
    void pump_callbacks();
    bool is_overlay_open();
    std::vector<uint8_t> get_encrypted_token();
    bool should_wait_for_encrypted_token();

private:
    callback_environment* secret_environment = nullptr;
};

#endif // STEAM_API_HPP_INCLUDED
