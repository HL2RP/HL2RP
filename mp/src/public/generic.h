#ifndef GENERIC_H
#define GENERIC_H
#pragma once

#define SCOPED_ENUM(Name, ...) \
struct _##Name \
{ \
	enum E \
	{ \
		__VA_ARGS__, \
		_Count \
	}; \
}; \
\
typedef _##Name::E Name;

template<class T>
class CDefaultGetAdapter : public T
{
public:
	template<typename K, typename V>
	V GetElementOrDefault(K key, V defaultValue = {})
	{
		int index = this->Find(key);
		return (this->IsValidIndex(index) ? (V&&)this->Element(index) : defaultValue);
	}
};

template<class T>
class CAutoLessFuncAdapter : public CDefaultGetAdapter<T>
{
public:
	CAutoLessFuncAdapter()
	{
		SetDefLessFunc(*this);
	}
};

template<class T>
class CAutoDeleteAdapter : public CDefaultGetAdapter<T>
{
public:
	~CAutoDeleteAdapter()
	{
		this->PurgeAndDeleteElements();
	}
};

// Allows expanding parameter packs from the caller when not possible
// directly (e.g. calling a function for each argument)
inline void PassVarArgs(...) {}

#endif // !GENERIC_H
