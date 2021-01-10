#ifndef GENERIC_H
#define GENERIC_H
#pragma once

#define ENUM(Name, ...) \
namespace Name \
{ \
	enum _Value \
	{ \
		__VA_ARGS__, \
		_Count \
	}; \
}

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

#endif // !GENERIC_H
