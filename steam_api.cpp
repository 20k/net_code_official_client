#include "steam_api.hpp"

#include <stdint.h>
#include <mutex>
#include <vector>
#include <iostream>

#include <toolkit/config.hpp>

#include <steamworks_sdk_142/sdk/public/steam/steam_api.h>
#include <steamworks_sdk_142/sdk/public/steam/isteamuser.h>

#ifndef NO_STEAM
struct callback_environment
{
    std::mutex lock;

    STEAM_CALLBACK( callback_environment, OnGameOverlayActivated, GameOverlayActivated_t );

    void OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure );
	CCallResult< callback_environment, EncryptedAppTicketResponse_t > m_OnRequestEncryptedAppTicketCallResult;

    bool overlay_open = false;

    bool auth_in_progress = false;

    bool has_encrypted_ticket = false;
    std::vector<uint8_t> encrypted_app_ticket;
};

void callback_environment::OnGameOverlayActivated( GameOverlayActivated_t* pCallback )
{
    std::lock_guard guard(lock);

    overlay_open = pCallback->m_bActive;
}

void callback_environment::OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure )
{
    std::lock_guard guard(lock);

    if ( pEncryptedAppTicketResponse->m_eResult == k_EResultOK )
	{
		uint32 cubTicket;
        encrypted_app_ticket.resize(1024);
		SteamUser()->GetEncryptedAppTicket( &encrypted_app_ticket[0], 1024, &cubTicket );

		encrypted_app_ticket.resize(cubTicket);

		has_encrypted_ticket = true;
		auth_in_progress = false;

		return;
	}
    else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultLimitExceeded )
	{
		printf("Hit rate limit (1/min)\n");

        uint32 cubTicket;
        encrypted_app_ticket.resize(1024);
		SteamUser()->GetEncryptedAppTicket( &encrypted_app_ticket[0], 1024, &cubTicket );

		encrypted_app_ticket.resize(cubTicket);

		has_encrypted_ticket = true;
		auth_in_progress = false;

		return;
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
    auth_in_progress = false;
}
#endif // NO_STEAM

steamapi::steamapi()
{
    #ifndef NO_STEAM
    enabled = SteamAPI_Init();
    secret_environment = new callback_environment;
    #endif // NO_STEAM

    std::cout << "steamapi support is " << enabled << std::endl;
}

void steamapi::pump_callbacks()
{
    if(!enabled)
        return;

    #ifndef NO_STEAM
    SteamAPI_RunCallbacks();
    #endif // NO_STEAM
}

void steamapi::request_auth_token(const std::string& user_data)
{
    if(!enabled)
        return;

    #ifndef NO_STEAM
    std::string lstr = user_data;

    SteamAPICall_t scall;

    if(lstr.size() > 0)
        scall = SteamUser()->RequestEncryptedAppTicket(&lstr[0], lstr.size());
    else
        scall = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);

    std::lock_guard guard(secret_environment->lock);
    secret_environment->auth_in_progress = true;
    secret_environment->m_OnRequestEncryptedAppTicketCallResult.Set( scall, secret_environment, &callback_environment::OnRequestEncryptedAppTicket );
    #endif // NO_STEAM
}

bool steamapi::auth_success()
{
    if(!enabled)
        return false;

    #ifndef NO_STEAM
    std::lock_guard guard(secret_environment->lock);

    if(secret_environment->auth_in_progress)
        return false;

    if(!secret_environment->has_encrypted_ticket)
        return false;

    return true;
    #else
    return false; //unreachable
    #endif
}

steamapi::~steamapi()
{
    #ifndef NO_STEAM
    if(enabled)
        SteamAPI_Shutdown();

    delete secret_environment;
    secret_environment = nullptr;
    #endif // NO_STEAM
}

bool steamapi::is_overlay_open()
{
    if(!enabled)
        return false;

    #ifndef NO_STEAM
    std::lock_guard guard(secret_environment->lock);

    return secret_environment->overlay_open;
    #else
    return false; //unreachable
    #endif
}

std::vector<uint8_t> steamapi::get_encrypted_token()
{
    #ifndef NO_STEAM
    std::lock_guard guard(secret_environment->lock);

    return secret_environment->encrypted_app_ticket;
    #else
    std::vector<uint8_t>();
    #endif
}

bool steamapi::should_wait_for_encrypted_token()
{
    #ifndef NO_STEAM
    std::lock_guard guard(secret_environment->lock);

    return secret_environment->auth_in_progress;
    #else
    return false;
    #endif
}
