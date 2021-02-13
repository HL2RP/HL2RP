#ifndef C_HL2_ROLEPLAYER_H
#define C_HL2_ROLEPLAYER_H
#pragma once

#include <hl2_roleplayer_shared.h>
#include <hl2rp_localizer.h>
#include <hl2rp_shareddefs.h>

class C_HL2Roleplayer : public CBaseHL2Roleplayer
{
	DECLARE_CLASS(C_HL2Roleplayer, CBaseHL2Roleplayer)
	DECLARE_HL2RP_NETWORKCLASS()

	CNetworkVar(int, m_iMaxHealth)

public:
	int GetMaxHealth() const OVERRIDE;

	void HandleWalkChanges();
	void LocalPrint(int type, const char*);
	void LocalDisplayHUDHint(EPlayerHUDHintType::_Value, const char*);
	bool ComputeAimingEntityAndHUD(localizebuf_t& dest);
};

C_HL2Roleplayer* GetLocalHL2Roleplayer();

#endif // !C_HL2_ROLEPLAYER_H
