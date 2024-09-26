// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include <hl2rp_localizer.h>
#include <hl2rp_shareddefs.h>
#include <c_ai_basenpc.h>

class C_HL2RPNPCPolice : public C_AI_BaseNPC
{
	DECLARE_CLASS(C_HL2RPNPCPolice, C_AI_BaseNPC)
	DECLARE_CLIENTCLASS()

	const char* GetDisplayName() OVERRIDE
	{
		return "#HL2RP_Police";
	}

	void GetHUDInfo(C_HL2Roleplayer*, CLocalizeFmtStr<>& text) OVERRIDE
	{
		text.Localize(GetDisplayName());
	}
};

LINK_ENTITY_TO_CLASS(npc_hl2rp_police, C_HL2RPNPCPolice)

IMPLEMENT_HL2RP_NETWORKCLASS(HL2RPNPCPolice)
END_RECV_TABLE()
