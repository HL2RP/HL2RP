// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cbase.h>
#include "idatabase_driver.h"
#include "idao.h"
#include "keyvalues_driver.h"
#include <filesystem.h>

void ISQLDriver::ExecuteQuery(CRecordListDTO* pDestResults, const char* pFormat, ...)
{
	char query[SQL_QUERY_MAX_SIZE];
	va_list args;
	va_start(args, pFormat);
	V_vsprintf_safe(query, pFormat, args);
	va_end(args);
	ExecuteQuery(query, pDestResults);
}

class CKeyValuesIOException : public CDatabaseIOException
{
public:
	CKeyValuesIOException(PRINTF_FORMAT_STRING const char* pWarningFormat, ...) FMTFUNCTION(2, 3)
	{
		Warning("KeyValues driver error: ");
		va_list args;
		va_start(args, pWarningFormat);
		WarningV(pWarningFormat, args);
		va_end(args);
		Warning("\n");
	}
};

CKeyValuesDriver::CKeyValuesDriver(const char* pRootPath)
{
	V_strcpy_safe(mRootPath, pRootPath);
}

void CKeyValuesDriver::DispatchExecuteIO(IDAO* pDAO)
{
	pDAO->ExecuteIO(this);
}

void CKeyValuesDriver::LoadFromFile(const char* pSubDirectory, const char* pBaseFileName,
	CKeyValuesDTOHandler* pResultsHandler)
{
	// Try loading the temporary file first, since in a normal execution it can only be more updated than permanent one
	CFmtStrN<MAX_PATH> path("%s/%s/%s.txt", mRootPath, pSubDirectory, pBaseFileName);
	LoadFromFile(path, pSubDirectory, pBaseFileName, pResultsHandler);
}

void CKeyValuesDriver::LoadFromFile(CFmtStrN<MAX_PATH>& path, const char* pSubDirectory,
	const char* pBaseFileName, CKeyValuesDTOHandler* pResultsHandler)
{
	int len = path.Length();
	path += ".tmp";
	KeyValuesAD results("");

	if (!results->LoadFromFile(filesystem, path))
	{
		path.SetLength(len);
		results->LoadFromFile(filesystem, path);
	}

	results->SetName(pSubDirectory); // Set correct root name here to fix possibly incorrect loaded one
	pResultsHandler->HandleLoadedFileData(results, pBaseFileName);
}

void CKeyValuesDriver::LoadFromDirectory(const char* pSubDirectory, CKeyValuesDTOHandler* pResultsHandler)
{
	CFmtStrN<MAX_PATH> searchPath("%s/%s/", mRootPath, pSubDirectory);

	if (filesystem->IsDirectory(searchPath))
	{
		int len = searchPath.Length();
		searchPath += "*";
		FileFindHandle_t findHandle;

		for (const char* pFileName = filesystem->FindFirst(searchPath, &findHandle);
			pFileName != NULL; pFileName = filesystem->FindNext(findHandle))
		{
			if (*pFileName != '.')
			{
				searchPath.SetLength(len);
				searchPath += pFileName;
				LoadFromFile(searchPath, pSubDirectory, pFileName, pResultsHandler);
			}
		}

		filesystem->FindClose(findHandle);
	}
}

void CKeyValuesDriver::SaveToFile(KeyValues* pData, const char* pSubDirectory, const char* pBaseFileName)
{
	CFmtStrN<MAX_PATH> path("%s/%s/", mRootPath, pSubDirectory);
	Q_strlower(path.Access());
	filesystem->CreateDirHierarchy(path);
	path.AppendFormat("%s.txt", pBaseFileName);
	int len = path.Length();
	path += ".tmp";

	if (!pData->SaveToFile(filesystem, path))
	{
		throw CKeyValuesIOException("Couldn't create file '%s' to save current data", path.Get());
	}

	char permPath[MAX_PATH]{};
	V_strcat_safe(permPath, path, len);

	// Delete current permanent file before moving temporary, since Windows doesn't allow renaming to an existing file
	if (filesystem->FileExists(permPath))
	{
		filesystem->RemoveFile(permPath);
	}

	filesystem->RenameFile(path, permPath);
}

void CKeyValuesDriver::DeleteFile(const char* pSubDirectory, const char* pBaseFileName)
{
	CFmtStrN<MAX_PATH> path("%s/%s/%s.txt", mRootPath, pSubDirectory, pBaseFileName);
	Q_strlower(path.Access());

	if (filesystem->FileExists(path))
	{
		filesystem->RemoveFile(path);
	}

	path += ".tmp";

	if (filesystem->FileExists(path))
	{
		filesystem->RemoveFile(path);
	}
}

void CKeyValuesDriver::DeleteDirectory(const char* pSubDirectory)
{
	CFmtStrN<MAX_PATH> searchPath("%s/%s/", mRootPath, pSubDirectory);
	Q_strlower(searchPath.Access());
	int len = searchPath.Length();
	searchPath += "*";
	FileFindHandle_t findHandle;

	for (const char* pFileName = filesystem->FindFirst(searchPath, &findHandle);
		pFileName != NULL; pFileName = filesystem->FindNext(findHandle))
	{
		if (*pFileName != '.')
		{
			searchPath.SetLength(len);
			searchPath += pFileName;
			filesystem->RemoveFile(searchPath);
		}
	}

	filesystem->FindClose(findHandle);
}
