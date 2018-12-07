#ifndef STEAMAPI_HPP_INCLUDED
#define STEAMAPI_HPP_INCLUDED

#include <libncclient/c_all.h>
#include <vector>
#include <cstdint>

struct callback_environment;

struct steamapi
{
    bool enabled = false;
    bool overlay_open = false;

    //std::vector<uint8_t> ticket;
    //uint32_t real_ticket_size = 0;

    //uint32_t hauthticket = 0;

    uint64_t steamapicall = 0;

    steamapi();
    ~steamapi();

    void handle_auth(c_shared_data shared);
    bool auth_success();
    void pump_callbacks();
    bool is_overlay_open();

private:
    callback_environment* secret_environment = nullptr;
};

#endif // STEAMAPI_HPP_INCLUDED
