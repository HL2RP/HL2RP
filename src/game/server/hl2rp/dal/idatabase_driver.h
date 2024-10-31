#ifndef IDATABASE_DRIVER_H
#define IDATABASE_DRIVER_H
#pragma once

#include <platform.h>

abstract_class IDAO;

abstract_class IDatabaseDriver
{
public:
	virtual ~IDatabaseDriver() = default;

	virtual void DispatchExecuteIO(IDAO*) = 0;
};

class CDatabaseIOException
{
public:
	virtual ~CDatabaseIOException() = default;
};

#endif // !IDATABASE_DRIVER_H
