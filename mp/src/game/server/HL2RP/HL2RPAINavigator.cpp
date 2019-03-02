#include <cbase.h>
#include "HL2RPAINavigator.h"
#include <ai_debug.h>
#include <ai_pathfinder.h>
#include <ai_route.h>
#include <doors.h>
#include <func_movelinear.h>
#include "HL2RPProperty.h"
#include "HL2RPRules.h"

// NPC distance from CBaseToggle entity origin at which to start opening it
#define HL2RP_AI_NAV_BASE_TOGGLE_NPC_OPEN_DIST	16.0f

CHL2RPAINavigator::CHL2RPAINavigator(CAI_BaseNPC * pOuter) : BaseClass(pOuter)
{

}

bool CHL2RPAINavigator::OnCalcBaseMove(AILocalMoveGoal_t* pMoveGoal, float distClear,
	AIMoveResult_t* pResult)
{
	CBaseEntity* pEntity = pMoveGoal->directTrace.pObstruction;

	if (pEntity != NULL
		&& (FClassnameIs(pEntity, "func_door") || FClassnameIs(pEntity, "func_movelinear")))
	{
		return OnUpcomingBaseToggle(static_cast<CBaseDoor*>(pEntity), pMoveGoal, distClear, pResult);
	}

	return BaseClass::OnCalcBaseMove(pMoveGoal, distClear, pResult);
}

bool CHL2RPAINavigator::OnMoveBlocked(AIMoveResult_t* pResult)
{
	if (*pResult == AIMR_BLOCKED_NPC && GetPath()->GetCurWaypoint() != NULL)
	{
		CBaseEntity* pEntity = GetPath()->GetCurWaypoint()->GetEHandleData();

		if (pEntity != NULL && pEntity == m_hBaseToggle)
		{
			NPCOpenDoor(GetOuter());
			*pResult = AIMR_OK;
			return true;
		}
	}

	return BaseClass::OnMoveBlocked(pResult);
}

void CHL2RPAINavigator::AdvancePath()
{
	if (m_hBaseToggle != NULL)
	{
		if (!CurWaypointIsGoal() && GetPath()->GetCurWaypoint()->GetEHandleData() == m_hBaseToggle)
		{
			NPCOpenDoor(GetOuter());
		}

		if (AIIsDebuggingDoors(GetOuter()))
		{
			NDebugOverlay::Line(GetOuter()->EyePosition(), m_hBaseToggle->WorldSpaceCenter(),
				255, 255, 255, false, 0.1f);
		}
	}

	BaseClass::AdvancePath();
}

bool CHL2RPAINavigator::OnUpcomingBaseToggle(CBaseToggle* pBaseToggle, AILocalMoveGoal_t* pMoveGoal,
	float distClear, AIMoveResult_t* pResult)
{
	if ((pMoveGoal->flags & AILMG_TARGET_IS_GOAL) && pMoveGoal->maxDist < distClear
		|| pMoveGoal->maxDist + GetHullWidth() * 0.25f < distClear)
	{
		// We are moving to the door already, don't calculate a path to it
		return false;
	}

	if (pBaseToggle == m_hBaseToggle)
	{
		// Is NPC opening door?
		if (m_hBaseToggle->m_hActivator != NULL && m_hBaseToggle->m_hActivator->IsNPC())
		{
			// We're in the process of opening the door, don't be blocked by it
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
			return true;
		}

		m_hBaseToggle = NULL;
	}

	// Check if we can and should go open the door
	if ((CapabilitiesGet() & bits_CAP_DOORS_GROUP) && !pBaseToggle->HasSpawnFlags(SF_DOOR_NONPCS)
		&& (pBaseToggle->m_toggle_state == TS_GOING_DOWN
			|| pBaseToggle->m_toggle_state == TS_AT_BOTTOM) && !pBaseToggle->IsLocked())
	{
		Vector standPos;

		// We assume that hit normal is the one of main door plane (hopefully)
		VectorMA(GetAbsOrigin(), HL2RP_AI_NAV_BASE_TOGGLE_NPC_OPEN_DIST,
			pMoveGoal->directTrace.vHitNormal, standPos);

		AI_Waypoint_t* pOpenDoorRoute = GetPathfinder()->BuildLocalRoute(GetLocalOrigin(), standPos,
			NULL, bits_WP_DONT_SIMPLIFY, NO_NODE, bits_BUILD_GROUND | bits_BUILD_IGNORE_NPCS, 0.0f);

		if (pOpenDoorRoute != NULL)
		{
			// Attach the door to the waypoint so we open it when we get there
			pOpenDoorRoute->m_hData = pBaseToggle;
			GetPath()->PrependWaypoints(pOpenDoorRoute);
			m_hBaseToggle = pBaseToggle;
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_CHANGE_TYPE;
			return true;
		}
	}

	return false; // Don't call BaseClass, as it wouldn't succeed with the current obstructing door
}

void CHL2RPAINavigator::NPCOpenDoor(CAI_BaseNPC* pNPC)
{
	variant_t variant;
	m_hBaseToggle->AcceptInput("Open", pNPC, NULL, variant, 0);
	m_hBaseToggle->m_hActivator = pNPC;

	// Wait for the door to finish opening before trying to move through the doorway
	pNPC->m_flMoveWaitFinished = gpGlobals->curtime + m_hBaseToggle->GetMoveDoneTime();
}

void CHL2RPAINavigator::OnDoorFullyOpen()
{
	m_hBaseToggle = NULL;
}
