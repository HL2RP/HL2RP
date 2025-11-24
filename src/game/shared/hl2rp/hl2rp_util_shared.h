#ifndef HL2RP_UTIL_SHARED_H
#define HL2RP_UTIL_SHARED_H
#pragma once

#include <string_t.h>
#include <utlstring.h>

#define INVALID_DATABASE_ID -1
#define LOADING_DATABASE_ID  0 // ID is still invalid but it's being loaded, so it can't be requested again

class KeyValues;

struct SUtlField
{
	enum class EType
	{
		Null, // For SQL NULL values saving (e.g. FK fields reset)
		Int,
		UInt64,
		Float,
		String
	};

	static SUtlField FromKeyValues(KeyValues*); // Converts the value tied to the KV

	SUtlField() : mType(EType::Null) {}
	SUtlField(int value) : mUInt64(value), mType(EType::Int) {}
	SUtlField(uint64 value) : mUInt64(value), mType(EType::UInt64) {}
	SUtlField(float value) : mFloat(value), mType(EType::Float) {}
	SUtlField(const char* pValue) : mString(pValue), mType(EType::String) {}

#ifndef NO_STRING_T
	SUtlField(const string_t& value) : SUtlField(STRING(value)) {}
#endif // !NO_STRING_T

	operator const char* () const;

	int ToInt() const;
	uint64 ToUInt64() const;
	float ToFloat() const;
	CUtlString ToString() const;

	union
	{
		int mInt;
		uint64 mUInt64 = 0;
		float mFloat;
	};

	CUtlConstString mString;
	EType mType;
};

#ifdef HL2RP // Exclude non-game libraries (e.g. SQL Drivers)
class CUtlPooledString
{
	const char* mpString;

public:
	CUtlPooledString(const char* = "");

	operator const char* ();
};

template<typename K = const char*>
using CUtlPooledStringMap = CAutoLessFuncAdapter<CUtlMap<K, CUtlPooledString>>;

// Numeric container with a minimum value of zero and few non-overflowing operations
template<typename T>
class CPositiveVarBase
{
	template<typename S>
	T Add(T value, S max)
	{
		if (value > 0)
		{
			return (mValue + Min(value, max - mValue));
		}

		return (mValue + Max(value, -mValue));
	}

	template<typename S>
	T Mult(T value, S max)
	{
		// Keep it simple even when not always reliable, for efficiency
		if (value > 0 && mValue > 0)
		{
			T result = mValue * value;
			return (result > mValue ? result : max);
		}

		return 0;
	}

	T mValue;

public:
	CPositiveVarBase(T value = 0) : mValue(value) {}

	operator T() const
	{
		return mValue;
	}

	T Get() const
	{
		return mValue;
	}

	template<typename S>
	T operator+(S value)
	{
		return Add(value, INT_MAX);
	}

	template<typename S>
	T operator-(S value)
	{
		return Add(-value, INT_MAX);
	}

	template<typename S>
	T operator*(S value)
	{
		return Mult(value, INT_MAX);
	}
};

template<>
template<typename S>
float CPositiveVarBase<float>::operator+(S value)
{
	return Add(value, FLT_MAX);
}

template<>
template<typename S>
float CPositiveVarBase<float>::operator-(S value)
{
	return Add(-value, FLT_MAX);
}

template<>
template<typename S>
float CPositiveVarBase<float>::operator*(S value)
{
	return Mult(value, FLT_MAX);
}

class CDefaultNetworkVarListener
{
public:
	static void NetworkVarChanged(...) {}
};

template<typename T = int, class Listener = CDefaultNetworkVarListener>
class CPositiveVar : public CNetworkVarBase<CPositiveVarBase<T>, Listener>
{
public:
	CPositiveVar(T value = 0) : CNetworkVarBase<CPositiveVarBase<T>, Listener>(value) {}

	template<typename S>
	T operator=(const S& value)
	{
		return this->Set(value);
	}

	operator T()
	{
		return this->Get();
	}
};

struct SDatabaseId
{
	SDatabaseId(int id = INVALID_DATABASE_ID);

	operator int();

#ifdef GAME_DLL
	operator SUtlField();

	bool SetForLoading(); // Returns true if ID was fully invalid
#endif // GAME_DLL

	bool IsValid();

private:
	int mId;
};

bool HL2RP_LoadConfigFile(KeyValues*, const char* pName); // NOTE: Resets current KeyValues first, for reusability

#ifdef HL2RP_CLIENT_OR_LEGACY
struct SRelativeTime
{
	SRelativeTime(int seconds);

	int mHours, mMinutes, mSeconds;
};

const char* UTIL_FormatDuration(CLocalizeFmtCStr&& dest, int seconds);
#endif // HL2RP_CLIENT_OR_LEGACY

const char* UTIL_FormatInteger(CBasePlayer*, int);

const char* UTIL_FormatMoney(CLocalizeFmtCStr& dest, int);
const char* UTIL_FormatMoney(CLocalizeFmtCStr&& dest, int);
#endif // HL2RP

#endif // !HL2RP_UTIL_SHARED_H
