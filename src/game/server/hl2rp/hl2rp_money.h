#ifndef HL2RP_MONEY_H
#define HL2RP_MONEY_H
#pragma once

#include <props.h>

class CMoneyProp : public CPhysicsProp
{
	DECLARE_CLASS(CMoneyProp, CPhysicsProp)
	DECLARE_DATADESC()

	void Precache() OVERRIDE;
	void Spawn() OVERRIDE;
	int ObjectCaps() OVERRIDE;
	void Use(CBaseEntity*, CBaseEntity*, USE_TYPE, float) OVERRIDE;

	color32 mRingEffectColor;

public:
	void StartEffects();
	void CreateRings();

	int mAmount;
};

#endif // !HL2RP_MONEY_H
