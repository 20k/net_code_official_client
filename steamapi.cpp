#include "steamapi.hpp"

#include <iostream>
#include <vector>

#include <stdint.h>
#include <steamworks_sdk_142/sdk/public/steam/steam_api.h>
#include <steamworks_sdk_142/sdk/public/steam/isteamuser.h>

struct callback_environment
{
    STEAM_CALLBACK( callback_environment, OnGameOverlayActivated, GameOverlayActivated_t );
    //STEAM_CALLBACK( callback_environment, OnAuthResponse, GetAuthSessionTicketResponse_t );

    void OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure );
	CCallResult< callback_environment, EncryptedAppTicketResponse_t > m_OnRequestEncryptedAppTicketCallResult;

    //STEAM_CALLRESULT( callback_environment, OnRequestEncryptedAppTicket, EncryptedAppTicketResponse_t );

    bool overlay_open = false;

    bool has_ticket = false;
    bool auth_finished = false;
    HAuthTicket hticket;

    bool has_encrypted_ticket = false;
    std::vector<uint8_t> encrypted_app_ticket;
};

void callback_environment::OnGameOverlayActivated( GameOverlayActivated_t* pCallback )
{
    overlay_open = pCallback->m_bActive;
}

void callback_environment::OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure )
{
    if ( pEncryptedAppTicketResponse->m_eResult == k_EResultOK )
	{
		//uint8 rgubTicket[1024];
		uint32 cubTicket;
        encrypted_app_ticket.resize(1024);
		SteamUser()->GetEncryptedAppTicket( &encrypted_app_ticket[0], 1024, &cubTicket );

		encrypted_app_ticket.resize(cubTicket);

		std::cout << "successfully got encrypted auth ticket of length " << encrypted_app_ticket.size() << std::endl;

		has_encrypted_ticket = true;

		return;
	}
    else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultLimitExceeded )
	{
		printf("Hit rate limit (1/min)\n");
	}
	else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultDuplicateRequest )
	{
		printf("Already a pending auth request\n");
	}
	else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultNoConnection )
	{
		printf("Steam not active\n");
	}

    std::cout << "Failed to get auth " << pEncryptedAppTicketResponse->m_eResult << std::endl;
}


/*void callback_environment::OnAuthResponse( GetAuthSessionTicketResponse_t* pResponse )
{
    if(pResponse->m_eResult == k_EResultOK)
    {
        hticket = pResponse->m_hAuthTicket;
        has_ticket = true;
    }

    auth_finished = true;
}*/

steamapi::steamapi()
{
    enabled = SteamAPI_Init();
    //ticket.resize(1024);

    secret_environment = new callback_environment;

    std::cout << "steamapi support is " << enabled << std::endl;
}

void steamapi::pump_callbacks()
{
    if(!enabled)
        return;

    SteamAPI_RunCallbacks();
}

void steamapi::handle_auth(c_shared_data shared)
{
    if(!enabled)
        return;

    SteamAPICall_t scall = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
    //hauthticket = SteamUser()->GetAuthSessionTicket(&ticket[0], ticket.size(), &real_ticket_size);

    secret_environment->m_OnRequestEncryptedAppTicketCallResult.Set( scall, secret_environment, &callback_environment::OnRequestEncryptedAppTicket );
}

bool steamapi::auth_success()
{
    if(!secret_environment->auth_finished)
        return false;

    if(secret_environment->auth_finished && !secret_environment->has_ticket)
        return false;

    return true;
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
