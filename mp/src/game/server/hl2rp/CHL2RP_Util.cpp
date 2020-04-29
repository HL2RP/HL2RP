#include "cbase.h"
#include "CHL2RP_Util.h"
#include "CHL2RP_Player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CHL2RP_Player *CHL2RP_Util::PlayerByNetworkIDString(const char *pszNetworkID)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2RP_Player *pPlayer = CHL2RP_Player::ToThisClassFast(CBaseEntity::Instance(i));

		if ((pPlayer != NULL) && !Q_strcmp(pszNetworkID, pPlayer->GetNetworkIDString()))
		{
			return pPlayer;
		}
	}

	return NULL;
}

void CHL2RP_Util::HudMessage(CHL2RP_Player *pToPlayer, int channel, float x, float y, float duration, const Color &color, const char *pMessage)
{
	hudtextparms_s tTextParam;
	tTextParam.channel = channel;
	tTextParam.x = x;
	tTextParam.y = y;
	tTextParam.r1 = color.r();
	tTextParam.g1 = color.g();
	tTextParam.b1 = color.b();
	tTextParam.effect = 0;
	tTextParam.holdTime = duration;
	tTextParam.fadeinTime = 0.0f;
	tTextParam.fadeoutTime = 0.0f;

	// Small heuristic to reduce flickering for cyclic channels:
	float elapsedTime = gpGlobals->curtime - pToPlayer->m_flNextHUDChannelTime[channel];

	if (elapsedTime < 0.1)
		tTextParam.holdTime += elapsedTime;

	UTIL_HudMessage(pToPlayer, tTextParam, pMessage);
	pToPlayer->m_flNextHUDChannelTime[channel] = gpGlobals->curtime + duration;
}

bool CHL2RP_Util::PointInside2DPolygon(const Vector2D &point, Vector2D vertexes[], int numVertexes)
{
	for (int i = 1; i < numVertexes - 1; i++)
	{
		// If the tree vertexes are same only check if point is there
		// Else the maths would lie:
		if (vertexes[0] == vertexes[i] && vertexes[i] == vertexes[i + 1])
		{
			if (point == vertexes[0])
				return true;
		}
		else
		{
			bool b1 = (point.x - vertexes[i].x) * (vertexes[0].y - vertexes[i].y) - (vertexes[0].x - vertexes[i].x) * (point.y - vertexes[i].y) <= 0.0f,
				b2 = (point.x - vertexes[i + 1].x) * (vertexes[i].y - vertexes[i + 1].y) - (vertexes[i].x - vertexes[i + 1].x) * (point.y - vertexes[i + 1].y) <= 0.0f,
				b3 = (point.x - vertexes[0].x) * (vertexes[i + 1].y - vertexes[0].y) - (vertexes[i + 1].x - vertexes[0].x) * (point.y - vertexes[0].y) <= 0.0f;

			if (b1 == b2 && b2 == b3)
			{
				return true;
			}
		}
	}

	return false;
}

void CHL2RP_Util::DisplayZone(IRecipientFilter &filter, Vector2D vertexes[], int numVertexes, int minHeight, int maxHeight, int modelIndex, float life, float width, float endWith, const Color &color)
{
	CRecipientFilter& _filter = ((CRecipientFilter &)filter);
	_filter.SetIgnorePredictionCull(true);

	for (int j = 0, next; j < numVertexes; j++)
	{
		next = (j + 1) % numVertexes;

		// Horizontal bottom:
		Vector start(vertexes[j].x, vertexes[j].y, minHeight), end(vertexes[next].x, vertexes[next].y, minHeight);

		te->BeamPoints(_filter, 0.0f, &start, &end, modelIndex, 0, 0, 0, life, width, endWith, 0, 0, color.r(), color.g(), color.b(), color.a(), 0);

		// Vertical:
		end = start;
		end.z = maxHeight;

		te->BeamPoints(_filter, 0.0f, &start, &end, modelIndex, 0, 0, 0, life, width, endWith, 0, 0, color.r(), color.g(), color.b(), color.a(), 0);

		// Horizontal top:
		start.x = vertexes[next].x;
		start.y = vertexes[next].y;
		start.z = maxHeight;

		te->BeamPoints(_filter, 0.0f, &start, &end, modelIndex, 0, 0, 0, life, width, endWith, 0, 0, color.r(), color.g(), color.b(), color.a(), 0);
	}
}
