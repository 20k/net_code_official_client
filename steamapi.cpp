#include "steamapi.hpp"

#include <iostream>

#include <stdint.h>
#include <steamworks_sdk_142/sdk/public/steam/steam_api.h>

steamapi::steamapi()
{
    enabled = SteamAPI_Init();

    std::cout << "steamapi support is " << enabled << std::endl;
}

steamapi::~steamapi()
{
    if(enabled)
        SteamAPI_Shutdown();
}
