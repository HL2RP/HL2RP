#include <cbase.h>
#include "DispenserMenu.h"
#include "AdminMenu.h"
#include <DAL/DispenserDAO.h>
#include <HL2RPRules.h>
#include <HL2RP_Player.h>
#include <DAL/IDAO.h>
#include <PropRationDispenser.h>
#include <util_shared.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

CDispenserMenu::CDispenserMenu(CHL2RP_Player* pPlayer) : CDialogRewindableMenu(pPlayer)
{
	AddItem(pPlayer, (new CDispenserCreateItem)->Link(this)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Dispenser_Create"));
	AddItem(pPlayer, (new CDispenserMoveCurrentItem)->Link(this)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Cur_Dispener_Move"));
	AddItem(pPlayer, (new CDispenserDeleteCurrentItem)->Link(this)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Cur_Dispenser_Rem"));
	AddItem(pPlayer, (new CDispenserDeleteAimedItem)->Link(this)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Aim_Dispenser_Rem"));
	AddItem(pPlayer, (new CDispenserSaveCurrentItem)->Link(this)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Cur_Dispenser_Save"));
	AddItem(pPlayer, (new CDispenserSaveAimedItem)->Link(this)->
		SetLocalizedDisplay(pPlayer, "#HL2RP_Menu_Item_Aim_Dispenser_Save"));
}

void CDispenserMenu::RewindDialog(CHL2RP_Player* pPlayer)
{
	pPlayer->DisplayDialog<CAdminMenu>();
}

CPropRationDispenser* CDispenserMenu::GetLookingDispenser(CHL2RP_Player* pPlayer)
{
	trace_t tr;
	const Vector& origin = pPlayer->EyePosition();
	const Vector& end = origin + pPlayer->EyeDirection3D() * MAX_TRACE_LENGTH;
	UTIL_TraceLine(origin, end, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr);
	return dynamic_cast<CPropRationDispenser*>(tr.m_pEnt);
}

void CDispenserMenu::TrySaveDispenser(CHL2RP_Player* pPlayer, CPropRationDispenser* pDispenser)
{
	if (CAdminMenu::CheckAccess(pPlayer) && pDispenser != NULL && pDispenser->m_iHammerID < 1)
	{
		if (pDispenser->m_iDatabaseId == IDAO_INVALID_DATABASE_ID)
		{
			TryCreateAsyncDAO<CDispenserFirstSaveDAO>(pDispenser);
		}
		else
		{
			TryCreateAsyncDAO<CDispenserFurtherSaveDAO>(pDispenser);
		}
	}
}

void CDispenserMenu::TryRemoveDispenser(CHL2RP_Player* pPlayer, CPropRationDispenser* pDispenser)
{
	if (CAdminMenu::CheckAccess(pPlayer) && pDispenser != NULL && pDispenser->m_iHammerID < 1)
	{
		TryCreateAsyncDAO<CDispenserDeleteDAO>(pDispenser);
		pDispenser->Remove();
	}
}

void CDispenserCreateItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		trace_t tr;
		const Vector& origin = pPlayer->EyePosition();
		const Vector& end = origin + pPlayer->EyeDirection3D() * MAX_TRACE_LENGTH;
		UTIL_TraceLine(origin, end, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.DidHit())
		{
			QAngle angles;
			VectorAngles(tr.plane.normal, angles);
			angles.x = angles.z = 0.0f;
			CBaseEntity* pEntity = CBaseEntity::Create("prop_ration_dispenser", tr.endpos, angles);
			m_pMenu->m_hDispenser = static_cast<CPropRationDispenser*>(pEntity);
		}

		pMenu->Display(pPlayer);
	}
}

void CDispenserMoveCurrentItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	if (CAdminMenu::CheckAccess(pPlayer))
	{
		if (m_pMenu->m_hDispenser != NULL)
		{
			trace_t tr;
			const Vector& origin = pPlayer->EyePosition();
			const Vector& end = origin + pPlayer->EyeDirection3D() * MAX_TRACE_LENGTH;

			UTIL_TraceLine(origin, end, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr);

			if (tr.DidHit())
			{
				QAngle angles;
				VectorAngles(tr.plane.normal, angles);
				angles.x = angles.z = 0.0f;

				m_pMenu->m_hDispenser->Teleport(&tr.endpos, &angles, NULL);
			}
		}

		pMenu->Display(pPlayer);
	}
}

void CDispenserDeleteCurrentItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	m_pMenu->TryRemoveDispenser(pPlayer, m_pMenu->m_hDispenser);
	pMenu->Display(pPlayer);
}

void CDispenserDeleteAimedItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	m_pMenu->TryRemoveDispenser(pPlayer, m_pMenu->GetLookingDispenser(pPlayer));
	pMenu->Display(pPlayer);
}

void CDispenserSaveCurrentItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	m_pMenu->TrySaveDispenser(pPlayer, m_pMenu->m_hDispenser);
	pMenu->Display(pPlayer);
}

void CDispenserSaveAimedItem::HandleSelection(CMenu* pMenu, CHL2RP_Player* pPlayer)
{
	m_pMenu->TrySaveDispenser(pPlayer, m_pMenu->GetLookingDispenser(pPlayer));
	pMenu->Display(pPlayer);
}
