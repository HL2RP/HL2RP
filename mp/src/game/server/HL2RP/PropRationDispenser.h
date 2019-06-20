#ifndef PROP_RATION_DISPENSER_H
#define PROP_RATION_DISPENSER_H

#include <props.h>

class CRation;
class CHL2RP_Player;

class CPropRationDispenser : public CDynamicProp
{
	DECLARE_CLASS(CPropRationDispenser, CDynamicProp)
	DECLARE_DATADESC();

	void Precache() OVERRIDE;
	void Spawn() OVERRIDE;

	bool IsOpen();
	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);
	void InputHandleClosed(inputdata_t& inputdata);

	CHandle<CRation> m_hContainedRation;
	int m_iSupplyGlowSpriteIndex, m_iDenyGlowSpriteIndex;
	float m_flNextUseableTime;
	float m_flNextEffectsReadyTime; // Used to avoid effects duplication (eg. feedback to criminals)

public:
	CPropRationDispenser();

	void CHL2RP_PlayerUse(const CHL2RP_Player& player);
	void OnContainedRationPickup();

	int m_iDatabaseId;
};

#endif // PROP_RATION_DISPENSER_H
