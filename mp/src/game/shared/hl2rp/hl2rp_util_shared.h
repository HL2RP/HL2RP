#ifndef HL2RP_UTIL_SHARED_H
#define HL2RP_UTIL_SHARED_H
#pragma once

#include <generic.h>

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

template<class T = CUtlDict<const char*>>
class CUtlStringDictionary : public CGenericAdapter<CAutoPurgeAdapter<T>>
{
public:
	int Insert(const char* pKey, const char* pValue)
	{
		return T::Insert(pKey, V_strdup(pValue));
	}

	void Remove(const char* pKey)
	{
		int index = T::Find(pKey);

		if (T::IsValidIndex(index))
		{
			delete T::Element(index);
			T::RemoveAt(index);
		}
	}
};

// For cases when all keys are externally allocated (e.g. global strings).
// NOTE: Calling InsertOrReplace won't re-allocate the string value, and may be unsafe on destruction.
using CUtlStaticKeyStringDictionary = CUtlStringDictionary<CAutoLessFuncAdapter<CUtlMap<const char*, const char*>>>;

struct SRelativeTime
{
	SRelativeTime(int seconds);

	int mHours, mMinutes, mSeconds;
};

#endif // !HL2RP_UTIL_SHARED_H