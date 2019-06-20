#ifndef HL2RP_AI_NAVIGATOR
#define HL2RP_AI_NAVIGATOR
#pragma once

#include <ai_navigator.h>

class CBaseToggle;

class CHL2RPAINavigator : public CAI_Navigator
{
	DECLARE_CLASS(CHL2RPAINavigator, CAI_Navigator);

	bool OnCalcBaseMove(AILocalMoveGoal_t* pMoveGoal, float distClear, AIMoveResult_t* pResult) OVERRIDE;
	bool OnMoveBlocked(AIMoveResult_t* pResult) OVERRIDE;
	void AdvancePath() OVERRIDE;

	bool OnUpcomingBaseToggle(CBaseToggle* pBaseToggle,
		AILocalMoveGoal_t* pMoveGoal, float distClear, AIMoveResult_t* pResult);
	void NPCOpenDoor(CAI_BaseNPC* pNPC);

	CHandle<CBaseToggle> m_hBaseToggle; // A door entity that is blocking our NPC

public:
	CHL2RPAINavigator(CAI_BaseNPC* pOuter);

	void OnDoorFullyOpen();
};

#endif // !HL2RP_AI_NAVIGATOR
