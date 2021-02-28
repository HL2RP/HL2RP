#ifndef HL2_ROLEPLAYER_H
#define HL2_ROLEPLAYER_H
#pragma once

#include <hl2_roleplayer_shared.h>
#include <hl2rp_localizer.h>
#include <hl2rp_shareddefs.h>
#include <bitflags.h>
#include <generic.h>
#include <smartptr.h>
#include <utlpair.h>

template<class Listener>
class CPlayerDatabasePropBase<CBitFlags<>, Listener> : public CBitFlags<CListenableNetworkVarBase<int, Listener>>
{
public:
	CPlayerDatabasePropBase(int flags = 0) : CBitFlags<CListenableNetworkVarBase<int, Listener>>(flags) {}
};

#define CPlayerDatabaseProp(Type, category, name) \
	class CPlayerDatabasePropListener_##name : public CDefaultNetworkVarListener \
	{ \
	public: \
		CPlayerDatabasePropListenerFunc(category, name) \
	}; \
\
	CPlayerDatabasePropBase<Type, CPlayerDatabasePropListener_##name> name;

class CCityZone;
class CHL2Roleplayer;

ENUM(EPlayerHUDType,
	Main,
	Alert,
	AimingEntity,
	Crime,
	Region,
	Phone,
	TypeCount
)

ENUM(EPlayerDatabaseMiscFlag, IsMOTDGuideSent)

ENUM(EPlayerDatabaseIOFlag,
	UpdateMainDataOnLoaded,
	UpdateAmmunitionOnLoaded,
	UpdateWeaponsOnLoaded,
	IsLoaded,
	IsEquipmentSaveDisabled // Active while restorable equipment exists
)

class CRestorablePlayerEquipment
{
	typedef CUtlPair<int, int> CWeaponClipsPair;

public:
	CRestorablePlayerEquipment(CHL2Roleplayer*);

	void Restore(CHL2Roleplayer*);

	int mHealth;
	CPositiveVar<> mArmor;
	CAutoLessFuncAdapter<CUtlMap<int, int>> mAmmoCountByIndex;
	CUtlDict<CWeaponClipsPair> mClipsByWeaponClassname;
};

class CHL2Roleplayer : public CBaseHL2Roleplayer
{
	DECLARE_CLASS(CHL2Roleplayer, CBaseHL2Roleplayer)
	DECLARE_HL2RP_NETWORKCLASS()

	void InitialSpawn() OVERRIDE;
	void Spawn() OVERRIDE;
	CBaseEntity* EntSelectSpawnPoint() OVERRIDE;
	void PostThink() OVERRIDE;
	bool PassesDamageFilter(const CTakeDamageInfo&) OVERRIDE;
	void Weapon_Drop(CBaseCombatWeapon*, const Vector*, const Vector*) OVERRIDE;
	void Event_Killed(const CTakeDamageInfo&) OVERRIDE;
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED(m_iMaxHealth);

	int TransferPrimaryAmmoFromWeapon(CBaseCombatWeapon*);
	int TransferSecondaryAmmoFromWeapon(CBaseCombatWeapon*);
	void HUDThink();
	void SendMainHUD() HL2RP_LEGACY_FUNCTION;
	bool AcquireHUDTime(EPlayerHUDType::_Value, bool force = false);
	void SendHUDMessage(EPlayerHUDType::_Value, const char* pMessage, float xPos, float yPos, const Color&);

	CSimpleSimTimer mHUDExpireTimers[EPlayerHUDType::TypeCount];

public:
	CHL2Roleplayer();

	void Weapon_Equip(CBaseCombatWeapon*) OVERRIDE;
	bool Weapon_EquipAmmoOnly(CBaseCombatWeapon*) OVERRIDE;
	void OnDatabasePropChanged(EPlayerDatabasePropType::_Value) OVERRIDE;

	void LoadFromDatabase();
	void HandleWalkChanges();
	void LocalPrint(int type, const char*) HL2RP_LEGACY_FUNCTION;
	void LocalDisplayHUDHint(EPlayerHUDHintType::_Value, const char*) HL2RP_LEGACY_FUNCTION;
	void Print(int type, const char*);
	void SendHUDHint(EPlayerHUDHintType::_Value, const char*);
	bool ComputeAimingEntityAndHUD(localizebuf_t& dest);

	CBitFlags<> mDatabaseIOFlags;
	CPlayerDatabaseProp(CBitFlags<>, EPlayerDatabasePropType::AccessFlags, mAccessFlags);
	CPlayerDatabaseProp(CBitFlags<>, EPlayerDatabasePropType::MiscFlags, mMiscFlags);
	CPlayerDatabaseProp(CBitFlags<>, EPlayerDatabasePropType::LearnedHUDHints, mLearnedHUDHints);
	CPlainAutoPtr<CRestorablePlayerEquipment> mRestorableCitizenEquipment;
	float mMOTDGuideSendReadyTime = FLT_MAX;
	CHandle<CCityZone> mhCityZone; // The zone that player is currently within
};

#endif // !HL2_ROLEPLAYER_H
