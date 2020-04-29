#ifndef HL2RP_UTIL_H
#define HL2RP_UTIL_H
#ifdef _WIN32
#pragma once
#endif

// CHL2RP_Util - Purpose: Provide global functions to be reused on varied contexts within the HL2RP

class CHL2RP_Player;
class Color;
class IRecipientFilter;

class CHL2RP_Util
{
public:
	static CHL2RP_Player *PlayerByNetworkIDString(const char *pszNetworkID);
	static void HudMessage(CHL2RP_Player *pToPlayer, int channel, float x, float y, float duration, const Color &color, const char *pMessage);
	static bool PointInside2DPolygon(const Vector2D &point, Vector2D vertexes[], int numVertexes);
	static void DisplayZone(IRecipientFilter &filter, Vector2D vertexes[], int numVertexes,
		int minHeight, int maxHeight, int modelIndex, float life, float width, float endWith, const Color &color);

	// Add up trying to avoid result overflowing
	FORCEINLINE static int EnsureAddition(int original, int amount)
	{
		if (amount > 0)
		{
			return (original < INT_MAX - amount) ? original + amount : INT_MAX;
		}
		else
		{
			return original;
		}
	};
};

#endif // HL2RP_UTIL_H
