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
class CGenericAdapter : public T
{
public:
	template<typename S>
	S GetElementOrDefault(const char* pName, S defaultValue = {})
	{
		int index = this->Find(pName);
		return (this->IsValidIndex(index) ? this->Element(index) : defaultValue);
	}
};

template<class T>
class CAutoPurgeAdapter : public T
{
public:
	~CAutoPurgeAdapter()
	{
		this->PurgeAndDeleteElements();
	}
};

template<class T>
class CAutoLessFuncAdapter : public T
{
public:
	CAutoLessFuncAdapter()
	{
		SetDefLessFunc(*this);
	}
};

// Allows expanding parameter packs from the caller when not possible
// directly (e.g. calling a function for each argument)
inline void PassVarArgs(...) {}

#endif // !GENERIC_H
