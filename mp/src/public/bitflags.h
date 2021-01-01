#ifndef BITFLAGS_H
#define BITFLAGS_H
#pragma once

#define FLAG_FOR_INDEX(index) (1 << (index))

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

	void SetFlag(int flag)
	{
		mFlags |= flag;
	}

	void SetBit(int index)
	{
		SetFlag(FLAG_FOR_INDEX(index));
	}

	template<typename... U>
	void SetBits(U... indices)
	{
		int indicesBuf[] = { indices... };

		for (auto index : indicesBuf)
		{
			SetBit(index);
		}
	}

	bool IsBitSet(int index) const
	{
		return ((mFlags & FLAG_FOR_INDEX(index)) > 0);
	}

	void ClearBit(int index)
	{
		mFlags &= ~FLAG_FOR_INDEX(index);
	}

	template<typename... U>
	void ClearBits(U... indices)
	{
		int indicesBuf[] = { indices... };

		for (auto index : indicesBuf)
		{
			ClearBit(index);
		}
	}
};

#endif // !BITFLAGS_H
