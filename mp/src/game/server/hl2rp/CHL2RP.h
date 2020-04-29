#ifndef ROLEPLAY_MAIN_H
#define ROLEPLAY_MAIN_H
#ifdef _WIN32
#pragma once
#endif

#include "CHL2RP_Zone.h"
#include "CHL2RP_SQL.h"

#ifdef HL2DM_RP
#include "CHL2RP_PlayerDialog.h"

enum CompatAnimModelIndex {
	INVALID_COMPAT_ANIM_INDEX = -1,
	COMPAT_ANIM_REBEL_MODEL_INDEX,
	COMPAT_ANIM_COMBINE_MODEL_INDEX,
	NUM_COMPAT_ANIM_MODEL_TYPES
};
#endif

class CHL2RP
{
private:
	static unsigned short FindChangedEntityOffset(const CBaseEntity &entity, void *pNetworkProp, int iNetworkPropValueSize,
		int iNetworkPropValueCount = 1, unsigned short i = 0, int *piValueIndexOut = NULL);

public:
	static void PostInit();
	static void ServerActivate();
	static void Think();
	template<class Handler>
	static void CheckEntityNetworkVarExChanged(const CBaseEntity &entity, Handler &networkVarEx);
	template<class Handler>
	static void CheckEntityNetworkArrayExChanged(const CBaseEntity &entity, Handler &networkArrayEx);

	static CUtlVector<CHL2RP_Zone> s_Zones;
	static CHL2RP_SQL *s_pSQL;

	//	"Phrases" (Key)
	//	{
	//		"Language code" (Key)
	//		{ 
	//			"Phrase header" "Phrase translation" (String value)
	//		}
	//	}
	static KeyValues *s_pPhrases;

#ifdef HL2DM_RP
	static int GetCompatModelTypeIndex(const char *model);

	static CUtlVector<IGameEvent *> s_SpecialDeathNoticeEvents;
	static CBasePlayerHandle s_hDeathNoticeBotHandle;
	static CUtlDict<CUtlString> s_CompatAnimationModelPaths[NUM_COMPAT_ANIM_MODEL_TYPES];
#endif
};

// Purpose: Wrap FindChangedEntityOffset for a passed CBaseEntity and CNetworkVarEx.
// If that function returns a valid index, trigger the NetworkStateChangedEx function indirectly
template<class Handler>
void CHL2RP::CheckEntityNetworkVarExChanged(const CBaseEntity &entity, Handler &networkVarEx)
{
	if ((entity.edict() != NULL) && entity.edict()->HasStateChanged()
		&& (FindChangedEntityOffset(entity, &networkVarEx, sizeof(networkVarEx.m_Value)) != MAX_CHANGE_OFFSETS))
	{
		networkVarEx.NetworkStateChanged(networkVarEx);
	}
}

// Purpose: Wrap FindChangedEntityOffset for a passed CBaseEntity and CNetworkArrayEx.
// If that function returns a valid index, trigger the NetworkStateChangedEx function indirectly
template<class Handler>
void CHL2RP::CheckEntityNetworkArrayExChanged(const CBaseEntity &entity, Handler &networkArrayEx)
{
	if ((entity.edict() != NULL) && entity.edict()->HasStateChanged())
	{
		for (unsigned short usLastIndex = 0;; usLastIndex++)
		{
			int iValueIndex;
			usLastIndex = FindChangedEntityOffset(entity, &networkArrayEx, sizeof(networkArrayEx.m_Value[0]),
				networkArrayEx.Count(), usLastIndex, &iValueIndex);

			if (usLastIndex == MAX_CHANGE_OFFSETS)
			{
				break;
			}

			networkArrayEx.NetworkStateChanged(iValueIndex, networkArrayEx.Get(iValueIndex));
		}
	}
}

#ifdef HL2DM_RP
abstract_class CCompatAnimModelTxn : public CAsyncSQLTxn
{
	DECLARE_CLASS(CCompatAnimModelTxn, CAsyncSQLTxn)

protected:
	CCompatAnimModelTxn(const char szCompatAnimModelID[]);
	bool ShouldBeReplacedBy(const BaseClass &txn) const OVERRIDE;

	char m_szCompatAnimModelID[MENU_ITEM_LENGTH];
};

class CSetUpCompatAnimModelsTxn : public CAsyncSQLTxn
{
public:
	CSetUpCompatAnimModelsTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
};

class CLoadCompatAnimModelsTxn : public CAsyncSQLTxn
{
public:
	CLoadCompatAnimModelsTxn();

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
	void HandleQueryResults() const OVERRIDE;
};

class CUpsertCompatAnimModelTxn : public CCompatAnimModelTxn
{
public:
	CUpsertCompatAnimModelTxn(const char *compatAnimModelID, const char *compatAnimModelPath, int compatAnimModelType);

private:
	void BuildSQLiteQueries() OVERRIDE;
	void BuildMySQLQueries() OVERRIDE;
	void BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const OVERRIDE;
	void BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const OVERRIDE;

	char compatAnimModelPath[MAX_PATH];
	int compatAnimModelType;
};

class CDeleteCompatAnimModelTxn : public CCompatAnimModelTxn
{
public:
	CDeleteCompatAnimModelTxn(const char *compatAnimModelID);

private:
	void BindParametersGeneric(int iQueryId, const void *pStmt) const OVERRIDE;
};
#endif

class CUserInputTxn : public CAsyncSQLTxn
{
public:
	CUserInputTxn(const char *query);

private:
	bool ShouldUsePreparedStatements() const OVERRIDE;
};

#endif // ROLEPLAY_MAIN_H
