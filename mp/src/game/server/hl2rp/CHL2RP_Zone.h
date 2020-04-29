#ifndef ROLEPLAY_ZONE_H
#define ROLEPLAY_ZONE_H

const int MAX_ZONE_POINTS = 6;

class CHL2RP_Zone
{
public:
	CHL2RP_Zone(const Vector &startOrigin);
	
	Vector2D m_vecPointsXY[MAX_ZONE_POINTS];
	float m_flMinHeight, m_flMaxHeight;
	int m_iNumPoints;
};

#endif // ROLEPLAY_ZONE_H
