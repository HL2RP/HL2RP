// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "hl2rp_util_shared.h"
#include <gamestringpool.h>

CUtlPooledString::CUtlPooledString(const char* pString) : mpString(STRING(AllocPooledString(pString)))
{

}

CUtlPooledString::operator const char* ()
{
	return mpString;
}

SDatabaseId::SDatabaseId(int id) : mId(id)
{

}

SDatabaseId::operator int()
{
	return mId;
}

bool SDatabaseId::IsValid()
{
	return (mId > LOADING_DATABASE_ID);
}

SRelativeTime::SRelativeTime(int seconds) : mHours(seconds / 3600), mMinutes(seconds / 60 % 60), mSeconds(seconds % 60)
{

}
