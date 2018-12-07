#ifndef STEAMAPI_HPP_INCLUDED
#define STEAMAPI_HPP_INCLUDED

#include <libncclient/c_all.h>

struct callback_environment;

struct steamapi
{
    bool enabled = false;
    bool overlay_open = false;

    steamapi();
    ~steamapi();

    bool handle_auth(c_shared_data shared);
    void pump_callbacks();
    bool is_overlay_open();

private:
    callback_environment* secret_environment = nullptr;
};

#endif // STEAMAPI_HPP_INCLUDED
