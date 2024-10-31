#ifndef HL2RP_UTIL_SHARED_H
#define HL2RP_UTIL_SHARED_H
#pragma once

#ifdef GAME_DLL
#include <hl2rp_util.h>
#endif // GAME_DLL

#define INVALID_DATABASE_ID -1
#define LOADING_DATABASE_ID  0 // ID is still invalid but it's being loaded, so it can't be requested again

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
		return (mValue + Min(value, max - mValue));
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
	CPositiveVarBase(T value = 0) : mValue(Max<T>(0, value)) {}

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

struct SRelativeTime
{
	SRelativeTime(int seconds);

	int mHours, mMinutes, mSeconds;
};

#endif // !HL2RP_UTIL_SHARED_H
