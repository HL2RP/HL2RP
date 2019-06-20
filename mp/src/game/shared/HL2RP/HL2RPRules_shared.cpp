#include <cbase.h>
#include <ammodef.h>

#ifdef CLIENT_DLL
#include <C_HL2RPRules.h>

#define CHL2RPRules	C_HL2RPRules
#else
#include <HL2RPRules.h>
#endif // CLIENT_DLL

#ifndef HL2DM_RP
REGISTER_GAMERULES_CLASS(CHL2RPRules);
#endif // !HL2DM_RP

void CHL2RPRules::SharedConstructor()
{
	CAmmoDef* pDef = GetAmmoDef();
	pDef->AddAmmoType("molotov", DMG_BURN, TRACER_NONE, 0, 0, INT_MAX, 0.0f,
		ITEM_FLAG_NOITEMPICKUP, 1, 1);
	pDef->AddAmmoType("ration", DMG_GENERIC, TRACER_NONE, 0, 0, INT_MAX, 0.0f,
		ITEM_FLAG_NOITEMPICKUP, 1, 1);
	pDef->AddAmmoType("suitcase", DMG_GENERIC, TRACER_NONE, 0, 0, 1, 0.0f,
		ITEM_FLAG_NOAMMOPICKUPS, 1, 1);
}
