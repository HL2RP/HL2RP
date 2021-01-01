// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include <ammodef.h>
#include <items.h>

#define VARIABLE_AMMO_ITEM_AUTO_REMOVE_DELAY 30.0f // Same value than weapons delay

class CVariableAmmoItem : public CItem
{
	DECLARE_CLASS(CVariableAmmoItem, CItem)

	void Precache() OVERRIDE;
	CBaseEntity* Respawn() OVERRIDE;
	bool MyTouch(CBasePlayer*) OVERRIDE;

public:
	int mAmmoIndex, mAmmoCount;
};

void CVariableAmmoItem::Precache()
{
	PrecacheModel("models/items/boxsrounds.mdl");
	PrecacheModel("models/items/357ammo.mdl");
	PrecacheModel("models/items/boxmrounds.mdl");
	PrecacheModel("models/items/ar2_grenade.mdl");
	PrecacheModel("models/items/combine_rifle_cartridge01.mdl");
	PrecacheModel("models/items/combine_rifle_ammo01.mdl");
	PrecacheModel("models/items/boxbuckshot.mdl");
	PrecacheModel("models/items/crossbowrounds.mdl");
}

CBaseEntity* CVariableAmmoItem::Respawn()
{
	SetTouch(&ThisClass::ItemTouch); // Restore disabled Touch by BaseClass
	return this;
}

bool CVariableAmmoItem::MyTouch(CBasePlayer* pPlayer)
{
	int givenAmmo = pPlayer->GiveAmmo(mAmmoCount, mAmmoIndex);

	if (givenAmmo > 0)
	{
		mAmmoCount -= givenAmmo;

		if (mAmmoCount < 1)
		{
			Remove();
		}

		return true;
	}

	return false;
}

LINK_ENTITY_TO_CLASS(item_variable_ammo, CVariableAmmoItem)
PRECACHE_REGISTER(item_variable_ammo);

static void CreateVariableAmmoItem(CBasePlayer* pPlayer, const char* pModelName, int index)
{
	int count = pPlayer->GetAmmoCount(index);

	if (count > 0)
	{
		CVariableAmmoItem* pItem = static_cast<CVariableAmmoItem*>
			(CBaseEntity::CreateNoSpawn("item_variable_ammo", pPlayer->GetAbsOrigin(), vec3_angle));
		pItem->PrecacheModel(pModelName);
		pItem->SetModel(pModelName);
		DispatchSpawn(pItem);
		pItem->mAmmoIndex = index;
		pItem->mAmmoCount = count;
		pItem->SetThink(&CBaseEntity::SUB_Remove);
		pItem->SetNextThink(gpGlobals->curtime + VARIABLE_AMMO_ITEM_AUTO_REMOVE_DELAY);
	}
}

void DropVariableAmmo(CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon)
{
	// For simplicity, we assume that no weapon uses both primary and secondary ammo
	if (pWeapon->UsesPrimaryAmmo())
	{
		int primaryAmmoType = pWeapon->GetPrimaryAmmoType();
		Ammo_t* pPrimaryAmmo = GetAmmoDef()->GetAmmoOfIndex(primaryAmmoType);

		if (Q_strcmp(pPrimaryAmmo->pName, "Pistol") == 0)
		{
			CreateVariableAmmoItem(pPlayer, "models/items/boxsrounds.mdl", primaryAmmoType);
		}
		else if (Q_strcmp(pPrimaryAmmo->pName, "357") == 0)
		{
			CreateVariableAmmoItem(pPlayer, "models/items/357ammo.mdl", primaryAmmoType);
		}
		else if (Q_strcmp(pPrimaryAmmo->pName, "SMG1") == 0)
		{
			CreateVariableAmmoItem(pPlayer, "models/items/boxmrounds.mdl", primaryAmmoType);
			CreateVariableAmmoItem(pPlayer, "models/items/ar2_grenade.mdl", pWeapon->GetSecondaryAmmoType());
		}
		else if (Q_strcmp(pPrimaryAmmo->pName, "AR2") == 0)
		{
			CreateVariableAmmoItem(pPlayer, "models/items/combine_rifle_cartridge01.mdl", primaryAmmoType);
			CreateVariableAmmoItem(pPlayer, "models/items/combine_rifle_ammo01.mdl", pWeapon->GetSecondaryAmmoType());
		}
		else if (Q_strcmp(pPrimaryAmmo->pName, "Buckshot") == 0)
		{
			CreateVariableAmmoItem(pPlayer, "models/items/boxbuckshot.mdl", primaryAmmoType);
		}
		else if (Q_strcmp(pPrimaryAmmo->pName, "XBowBolt") == 0)
		{
			CreateVariableAmmoItem(pPlayer, "models/items/crossbowrounds.mdl", primaryAmmoType);
		}
		else
		{
			// We don't have a designated item for primary ammo, so, transfer player's reserve ammo directly
			int ammoCount = pPlayer->GetAmmoCount(primaryAmmoType);
			pWeapon->SetPrimaryAmmoCount(ammoCount);
			pPlayer->RemoveAmmo(ammoCount, primaryAmmoType);
		}
	}
	else if (pWeapon->UsesSecondaryAmmo())
	{
		int secondaryAmmoType = pWeapon->GetSecondaryAmmoType();
		int ammoCount = pPlayer->GetAmmoCount(secondaryAmmoType);
		pWeapon->SetSecondaryAmmoCount(ammoCount);
		pPlayer->RemoveAmmo(ammoCount, secondaryAmmoType);
	}
}
