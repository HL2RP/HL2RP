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
#include <hl2_roleplayer.h>
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
	CHL2Roleplayer* pPlayer = ToHL2Roleplayer(CBaseEntity::Instance(pEntity));

	if (pPlayer != NULL)
	{
		data->SetInt("level", --pPlayer->mLastDialogLevel); // Just force our next level
	}
#endif // HL2RP

	// return false here to disallow a plugin from running this command on this client
	return true;
}

