// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "c_hl2rp_gamerules.h"
#include "c_hl2_roleplayer.h"
#include <hl2rp_configuration.h>
#include <hl2rp_property.h>
#include <c_user_message_register.h>

static void __MsgFunc_HL2RP_SavePlayerName(bf_read& msg)
{
	uint64 steamIdNumber = msg.ReadVarInt64();
	char name[MAX_PLAYER_NAME_LENGTH];
	msg.ReadString(name, sizeof(name));
	HL2RPRules()->mPlayerNameBySteamIdNum.InsertOrReplace(steamIdNumber, name);
}

static void __MsgFunc_HL2RP_PropertyUpdated(bf_read& msg)
{
	auto& propertyById = HL2RPRules()->mPropertyById;
	int id = msg.ReadLong(), index = propertyById.Find(id);

	if (propertyById.IsValidIndex(index))
	{
		if (msg.GetNumBytesLeft() < 1)
		{
			delete propertyById[index];
			return propertyById.RemoveAt(index);
		}
	}
	else if (msg.GetNumBytesLeft() < 1)
	{
		return;
	}
	else
	{
		index = propertyById.Insert(id, new CHL2RP_Property);
	}

	propertyById[index]->mType = (EHL2RP_PropertyType)msg.ReadByte();
	msg.ReadString(propertyById[index]->mName, sizeof(propertyById[index]->mName));
	propertyById[index]->mOwnerSteamIdNumber = msg.ReadVarInt64();

	if (!propertyById[index]->HasOwner())
	{
		propertyById[index]->mGrantedSteamIdNumbers.Purge();
	}
}

static void __MsgFunc_HL2RP_PropertyGrantUpdated(bf_read& msg)
{
	auto& propertyById = HL2RPRules()->mPropertyById;
	int index = propertyById.Find(msg.ReadLong());

	if (propertyById.IsValidIndex(index))
	{
		(msg.ReadOneBit() > 0) ? propertyById[index]->mGrantedSteamIdNumbers.InsertIfNotFound(msg.ReadVarInt64())
			: propertyById[index]->mGrantedSteamIdNumbers.Remove(msg.ReadVarInt64());
	}
}

static void __MsgFunc_HL2RP_CityZoneUpdated(bf_read& msg)
{
	auto& zoneByIndex = HL2RPRules()->mCityZoneByIndex;
	int entryIndex = msg.ReadLong(), index = zoneByIndex.Find(entryIndex);

	if (zoneByIndex.IsValidIndex(index))
	{
		if (msg.GetNumBytesLeft() < 1)
		{
			if (zoneByIndex[index] != NULL)
			{
				zoneByIndex[index]->Remove();
			}

			return zoneByIndex.RemoveAt(index);
		}
	}
	else if (msg.GetNumBytesLeft() < 1)
	{
		return;
	}
	else
	{
		index = zoneByIndex.Insert(entryIndex);
	}

	if (zoneByIndex[index] == NULL)
	{
		zoneByIndex[index] = static_cast<CCityZone*>(ClientEntityList()
			.AddNonNetworkableEntity(CreateEntityByName("trigger_city_zone")).Get());
	}

	zoneByIndex[index]->mpProperty = HL2RPRules()->mPropertyById
		.GetElementOrDefault<int, CHL2RP_Property*>(msg.ReadLong());
}

USER_MESSAGE_REGISTER(HL2RP_SavePlayerName)
USER_MESSAGE_REGISTER(HL2RP_PropertyUpdated)
USER_MESSAGE_REGISTER(HL2RP_PropertyGrantUpdated)
USER_MESSAGE_REGISTER(HL2RP_CityZoneUpdated)

C_HL2RPRules::C_HL2RPRules()
{
	ListenForGameEvent("player_activate");
	ListenForGameEvent("player_spawn");
}

void C_HL2RPRules::LevelShutdownPostEntity()
{
	mPlayerNameBySteamIdNum.Purge();
	mPropertyById.PurgeAndDeleteElements();
	mCityZoneByIndex.Purge();
}

void C_HL2RPRules::FireGameEvent(IGameEvent* pEvent)
{
	if (Q_strcmp(pEvent->GetName(), "player_activate") == 0)
	{
		// Notify the server of current learned HUD hints, to save server from sending these again
		engine->ServerCmdKeyValues(new KeyValues(HL2RP_LEARNED_HUD_HINTS_UPDATE_EVENT,
			HL2RP_LEARNED_HUD_HINTS_FIELD_NAME,
			gHL2RPConfiguration.mUserData->GetInt(HL2RP_LEARNED_HUD_HINTS_FIELD_NAME)));
	}
	else if (Q_strcmp(pEvent->GetName(), "player_spawn") == 0)
	{
		C_BasePlayer* pPlayer = UTIL_PlayerByUserId(pEvent->GetInt("userid"));

		if (pPlayer != NULL)
		{
			ToHL2Roleplayer(pPlayer)->mAddIntersectingEnts = true;
		}
	}
}
