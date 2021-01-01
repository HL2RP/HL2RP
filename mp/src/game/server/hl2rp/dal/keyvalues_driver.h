#ifndef KEYVALUES_DRIVER_H
#define KEYVALUES_DRIVER_H
#pragma once

#include "idatabase_driver.h"
#include <fmtstr.h>

class CKeyValuesDTOHandler;

class CKeyValuesDriver : public IDatabaseDriver
{
	void LoadFromFile(CFmtStrN<MAX_PATH>& path, const char* pSubDirectory,
		const char* pBaseFileName, CKeyValuesDTOHandler* pResultsHandler);

	char mRootPath[MAX_PATH];

public:
	CKeyValuesDriver(const char* pRootPath);

	void DispatchExecuteIO(IDAO*) OVERRIDE;

	void LoadFromFile(const char* pSubDirectory, const char* pBaseFileName, CKeyValuesDTOHandler* pResultsHandler);
	void LoadFromDirectory(const char* pSubDirectory, CKeyValuesDTOHandler* pResultsHandler);
	void SaveToFile(KeyValues* pData, const char* pSubDirectory, const char* pBaseFileName);
	void DeleteFile(const char* pSubDirectory, const char* pBaseFileName);
	void DeleteDirectory(const char* pSubDirectory);
};

#endif // !KEYVALUES_DRIVER_H
