//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// This class allows the game dll to control what functions 3rd party plugins can 
//  call on clients.
//
//=============================================================================//

#include "cbase.h"
#include "eiface.h"

#ifdef HL2RP
#include <HL2RP_Player.h>
#endif // HL2RP

//-----------------------------------------------------------------------------
// Purpose: An implementation 
//-----------------------------------------------------------------------------
class CPluginHelpersCheck : public IPluginHelpersCheck
{
public:
	virtual bool CreateMessage( const char *plugin, edict_t *pEntity, DIALOG_TYPE type, KeyValues *data );
};

CPluginHelpersCheck s_PluginCheck;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPluginHelpersCheck, IPluginHelpersCheck, INTERFACEVERSION_PLUGINHELPERSCHECK, s_PluginCheck);

bool CPluginHelpersCheck::CreateMessage( const char *plugin, edict_t *pEntity, DIALOG_TYPE type, KeyValues *data )
{
#ifdef HL2RP
	// For now, assume target is either a player or NULL
	CHL2RP_Player *pPlayer = ToHL2RPPlayer_Fast(CBaseEntity::Instance(pEntity));

	if (pPlayer != NULL)
	{
		// Enforce level to be set decremented from SDK, which should start from INT_MAX - 1.
		// This way no values are skipped accross messages and usages are maximized.
		pPlayer->m_iLastDialogLevel = Min(data->GetInt("level"), pPlayer->m_iLastDialogLevel);
		data->SetInt("level", --pPlayer->m_iLastDialogLevel);
	}
#endif // HL2RP

	// Return false here to disallow a plugin from running this command on this client
	return true;
}

