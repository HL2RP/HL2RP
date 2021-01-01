#ifndef HL2_ROLEPLAYER_H
#define HL2_ROLEPLAYER_H
#pragma once

#include <hl2_roleplayer_shared.h>
#include <hl2rp_localize.h>
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
	void Weapon_Drop(CBaseCombatWeapon*, const Vector*, const Vector*) OVERRIDE;
	void Event_Killed(const CTakeDamageInfo&) OVERRIDE;

	int TransferPrimaryAmmoFromWeapon(CBaseCombatWeapon*);
	int TransferSecondaryAmmoFromWeapon(CBaseCombatWeapon*);
	void HUDThink();
	void SendMainHUD();
	bool AcquireHUDTime(EPlayerHUDType::Value, bool force = false);
	void SendHUDMessage(EPlayerHUDType::Value, const char* pMessage, float xPos, float yPos, const Color&);

	CSimpleSimTimer mHUDExpireTimers[EPlayerHUDType::TypeCount];

public:
	CHL2Roleplayer();

	void Weapon_Equip(CBaseCombatWeapon*) OVERRIDE;
	bool Weapon_EquipAmmoOnly(CBaseCombatWeapon*) OVERRIDE;
	void OnDatabasePropChanged(EPlayerDatabasePropType::Value) OVERRIDE;

	void LoadFromDatabase();
	void HandleWalkChanges();
	void SendHUDHint(EPlayerHUDHintType::Value, const char*, bool networked = true);
	bool ComputeAimingEntityAndHUD(localizebuf_t& dest);

	CBitFlags<> mDatabaseIOFlags;
	CPlayerDatabaseProp(CBitFlags<>, EPlayerDatabasePropType::AccessFlags, mAccessFlags);
	CPlayerDatabaseProp(CBitFlags<>, EPlayerDatabasePropType::MiscFlags, mMiscFlags);
	CPlayerDatabaseProp(CBitFlags<>, EPlayerDatabasePropType::SentHUDHints, mSentHUDHints);
	CPlainAutoPtr<CRestorablePlayerEquipment> mRestorableCitizenEquipment;
	float mMOTDGuideSendReadyTime = FLT_MAX;
	CHandle<CCityZone> mhCityZone; // The zone that player is currently within
};

#endif // !HL2_ROLEPLAYER_H
