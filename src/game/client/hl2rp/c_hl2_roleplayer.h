#ifndef C_HL2_ROLEPLAYER_H
#define C_HL2_ROLEPLAYER_H
#pragma once

#include <hl2_roleplayer_shared.h>

#define HL2_ROLEPLAYER_JOB_NAME_SIZE 32

class C_HL2Roleplayer : public CBaseHL2Roleplayer
{
	DECLARE_CLASS(C_HL2Roleplayer, CBaseHL2Roleplayer)
	DECLARE_CLIENTCLASS()
	DECLARE_PREDICTABLE();

	const char* GetDisplayName() OVERRIDE;
	void PostThink() OVERRIDE;
	void GetHUDInfo(C_HL2Roleplayer*, CLocalizeFmtStr<>&) OVERRIDE;

	int m_iMaxHealth;
	char mJobName[HL2_ROLEPLAYER_JOB_NAME_SIZE];

public:
	int GetMaxHealth() const OVERRIDE;
	void HandleWalkChanges(CMoveData*) OVERRIDE;

	void LocalPrint(int type, const char* pText, const char* pArg = NULL);
	void LocalDisplayHUDHint(EPlayerHUDHintType, const char* pToken, const char* pArg1 = "", const char* pArg2 = "");
	void GetMainHUD(CLocalizeFmtStr<>&);

	bool mAddIntersectingEnts = false; // To prevent prediction breaking timer from player_spawn event otherwise
};

C_HL2Roleplayer* GetLocalHL2Roleplayer();

#endif // !C_HL2_ROLEPLAYER_H
