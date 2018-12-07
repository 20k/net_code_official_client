#include "steamapi.hpp"

#include <iostream>
#include <vector>

#include <stdint.h>
#include <steamworks_sdk_142/sdk/public/steam/steam_api.h>
#include <steamworks_sdk_142/sdk/public/steam/isteamuser.h>

struct callback_environment
{
    STEAM_CALLBACK( callback_environment, OnGameOverlayActivated, GameOverlayActivated_t );
    STEAM_CALLBACK( callback_environment, OnAuthResponse, GetAuthSessionTicketResponse_t );

    bool overlay_open = false;
};

void callback_environment::OnGameOverlayActivated( GameOverlayActivated_t* pCallback )
{
    overlay_open = pCallback->m_bActive;
}

void callback_environment::OnAuthResponse( GetAuthSessionTicketResponse_t* pResponse )
{

}

steamapi::steamapi()
{
    enabled = SteamAPI_Init();

    secret_environment = new callback_environment;

    std::cout << "steamapi support is " << enabled << std::endl;
}

void steamapi::pump_callbacks()
{
    if(!enabled)
        return;

    SteamAPI_RunCallbacks();
}

bool steamapi::handle_auth(c_shared_data shared)
{
    if(!enabled)
        return false;

    std::vector<uint8_t> ticket;
    ticket.resize(1024);

    uint32 real_ticket_size = 0;

    HAuthTicket result = SteamUser()->GetAuthSessionTicket(&ticket[0], ticket.size(), &real_ticket_size);

    return false;
}

steamapi::~steamapi()
{
    if(enabled)
        SteamAPI_Shutdown();

    delete secret_environment;
}

bool steamapi::is_overlay_open()
{
    return secret_environment->overlay_open;
}
