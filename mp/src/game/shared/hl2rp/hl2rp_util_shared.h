#ifndef HL2RP_UTIL_SHARED_H
#define HL2RP_UTIL_SHARED_H
#pragma once

#include <listenablenetworkvar.h>

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

template<typename T = int, class Listener = CDefaultNetworkVarListener>
class CPositiveVar : public CListenableNetworkVarBase<CPositiveVarBase<T>, Listener>
{
public:
	CPositiveVar(T value = 0) : CListenableNetworkVarBase<CPositiveVarBase<T>, Listener>(value) {}

	operator T()
	{
		return this->Get();
	}
};

#endif // !HL2RP_UTIL_SHARED_H
