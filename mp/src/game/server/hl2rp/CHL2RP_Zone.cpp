#include "cbase.h"
#include "CHL2RP_Zone.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CHL2RP_Zone::CHL2RP_Zone(const Vector &startOrigin) :
m_flMinHeight(startOrigin.z), m_flMaxHeight(startOrigin.z), m_iNumPoints(0)
{

}