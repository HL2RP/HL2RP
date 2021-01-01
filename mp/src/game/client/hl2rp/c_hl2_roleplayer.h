#ifndef C_HL2_ROLEPLAYER_H
#define C_HL2_ROLEPLAYER_H
#pragma once

#include <hl2_roleplayer_shared.h>
#include <hl2rp_localize.h>
#include <hl2rp_shareddefs.h>

class C_HL2Roleplayer : public CBaseHL2Roleplayer
{
	DECLARE_CLASS(C_HL2Roleplayer, CBaseHL2Roleplayer)
	DECLARE_HL2RP_NETWORKCLASS()

	void SendHUDHint(EPlayerHUDHintType::Value, const char*, bool);

public:
	void HandleWalkChanges();
	bool ComputeAimingEntityAndHUD(localizebuf_t& dest);
};

C_HL2Roleplayer* GetLocalHL2Roleplayer();

#endif // !C_HL2_ROLEPLAYER_H
