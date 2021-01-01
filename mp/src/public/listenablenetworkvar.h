#ifndef LISTENABLENETWORKVAR_H
#define LISTENABLENETWORKVAR_H
#pragma once

#define PointerOf(Class, pVar, varName) ((Class*)((char*)pVar - MyOffsetOf(Class, varName)))

class CDefaultNetworkVarListener
{
public:
	static void NetworkStateChanged(void*) {}
	static void OnValueChanged(void*) {}
};

template<typename T, class Listener = CDefaultNetworkVarListener>
class CListenableNetworkVarBase
{
	T mValue;

public:
	explicit CListenableNetworkVarBase(const T& value = 0) : mValue(value) {}

	operator const T& () const
	{
		return mValue;
	}

	const T& Get() const
	{
		return mValue;
	}

	const T* operator->() const
	{
		return &mValue;
	}

	template<typename S>
	const T& operator=(const S& value)
	{
		return Set(value);
	}

	template<typename S>
	const T& operator=(const CListenableNetworkVarBase<S, Listener>& value)
	{
		return Set(value.mValue);
	}

	template<typename S>
	const T& operator+=(const S& value)
	{
		return Set(mValue + value);
	}

	template<typename S>
	const T& operator-=(const S& value)
	{
		return Set(mValue - value);
	}

	template<typename S>
	const T& operator*=(const S& value)
	{
		return Set(mValue * value);
	}

	template<typename S>
	const T& operator/=(const S& value)
	{
		return Set(mValue / value);
	}

	template<typename S>
	const T& operator^=(const S& value)
	{
		return Set(mValue ^ value);
	}

	template<typename S>
	const T& operator|=(const S& value)
	{
		return Set(mValue | value);
	}

	const T& operator++()
	{
		return Set(mValue + 1);
	}

	T operator--()
	{
		return Set(mValue - 1);
	}

	T operator++(int)
	{
		T value = mValue;
		Set(mValue + 1);
		return value;
	}

	T operator--(int)
	{
		T value = mValue;
		Set(mValue - 1);
		return value;
	}

	template<typename S>
	const T& operator&=(const S& value)
	{
		return Set(mValue & value);
	}

	const T& Set(const T& value)
	{
		if (value != mValue)
		{
			Listener::NetworkStateChanged(this);
			mValue = value;
			Listener::OnValueChanged(this);
		}

		return mValue;
	}
};

#define CInternalListenableNetworkVar(Type, name, stateChangedFn) \
	NETWORK_VAR_START(Type, name) \
		static void OnValueChanged(void* pVar) \
		{ \
			PointerOf(ThisClass, pVar, name)->OnValueChanged_##name(); \
		} \
	NETWORK_VAR_END(Type, name, CListenableNetworkVarBase, stateChangedFn) \
\
	void OnValueChanged_##name();

#define CListenableNetworkVar(Type, name) CInternalListenableNetworkVar(Type, name, NetworkStateChanged)

#define CListenableNetworkVarForDerived(Type, name) \
	DISABLE_NETWORK_VAR_FOR_DERIVED(name); \
	CInternalListenableNetworkVar(Type, name, NetworkStateChanged_##name)

template<typename T, class Listener, int count>
class CListenableNetworkArrayBase
{
	T mElements[count];

public:
	template<typename S> friend int ServerClassInit(S*);

	CListenableNetworkArrayBase()
	{
		for (auto& value : mElements)
		{
			NetworkVarConstruct(value);
		}
	}

	const T* Base() const
	{
		return mElements;
	}

	int Count() const
	{
		return count;
	}

	const T& Get(int index) const
	{
		Assert(index >= 0 && index < count);
		return mElements[index];
	}

	const T& operator[](int index) const
	{
		return Get(index);
	}

	void Set(int index, const T& value)
	{
		Assert(index >= 0 && index < count);

		if (mElements[index] != value)
		{
			CHECK_USENETWORKVARS
			{
				Listener::NetworkStateChanged(this, mElements + index);
			}

			T oldValue = mElements[index];
			mElements[index] = value;
			Listener::OnElementChanged(this, index, oldValue);
		}
	}
};

#define CInternalListenableNetworkArray(Type, name, count, stateChangedFn) \
	class CListenableNetworkArrayListener_##name \
	{ \
	public: \
		static void NetworkStateChanged(void *pVar, void *pElement) \
		{ \
			PointerOf(ThisClass, pVar, name)->stateChangedFn(pElement); \
		} \
\
		static void OnElementChanged(void *pVar, int index, const Type& oldValue) \
		{ \
			PointerOf(ThisClass, pVar, name)->OnElementChanged_##name(index, oldValue); \
		} \
	}; \
\
	friend class CListenableNetworkArrayBase_##name; \
	typedef ThisClass MakeANetworkVar_##name; \
	typedef CListenableNetworkArrayBase<Type, CListenableNetworkArrayListener_##name, count> \
		CListenableNetworkArrayBase_##name; \
\
	void OnElementChanged_##name(int index, const Type& oldValue); \
\
	CListenableNetworkArrayBase_##name name;

#define CListenableNetworkArray(Type, name, count) \
	CInternalListenableNetworkArray(Type, name, count, NetworkStateChanged)

#define CListenableNetworkArrayForDerived(Type, name, count) \
	DISABLE_NETWORK_VAR_FOR_DERIVED(name); \
	CInternalListenableNetworkArray(Type, name, count, NetworkStateChanged_##name)

#endif // !LISTENABLENETWORKVAR_H
