#ifndef BITFLAGS_H
#define BITFLAGS_H
#pragma once

#include <initializer_list>

#define INDEX_TO_FLAG(index) (1 << (index))

// Class that simplifies handling flags by using bit indices instead of masks.
// This allows one to employ a related enum made up by automatically assigned indices.
template<typename T = int>
class CBitFlags
{
	T mFlags;

public:
	CBitFlags(int flags = 0) : mFlags(flags) {}

	operator int()
	{
		return mFlags;
	}

	int Get()
	{
		return mFlags;
	}

	bool IsBitSet(int index) const
	{
		return !!(mFlags & INDEX_TO_FLAG(index));
	}

	template<typename... U>
	bool IsAnyBitSet(U... indices)
	{
		for (auto index : { indices... })
		{
			if (IsBitSet(index))
			{
				return true;
			}
		}

		return false;
	}

	template<typename... U>
	bool AreAllBitsSet(U... indices)
	{
		for (auto index : { indices... })
		{
			if (!IsBitSet(index))
			{
				return false;
			}
		}

		return true;
	}

	void SetBit(int index)
	{
		SetFlag(INDEX_TO_FLAG(index));
	}

	template<typename... U>
	void SetBits(U... indices)
	{
		for (auto index : { indices... })
		{
			SetBit(index);
		}
	}

	void SetFlag(int flag)
	{
		mFlags |= flag;
	}

	void ClearBit(int index)
	{
		mFlags &= ~INDEX_TO_FLAG(index);
	}

	template<typename... U>
	void ClearBits(U... indices)
	{
		for (auto index : { indices... })
		{
			ClearBit(index);
		}
	}
};

#endif // !BITFLAGS_H
