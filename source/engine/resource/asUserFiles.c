#include "asUserFiles.h"

#include "SDL_filesystem.h"

size_t prefPathLength;
char* prefPath;

ASEXPORT void asInitUserFiles(const char* developerName, const char* appName)
{
	ASASSERT(!prefPath);

	prefPath = SDL_GetPrefPath(developerName, appName);

	if (!prefPath)
	{
		asFatalError("User Save Path Not Availible");
	}
	asDebugLog("User Save Directory: %s", prefPath);
	prefPathLength = strlen(prefPath);
}

ASEXPORT void asShutdownUserFiles()
{
	SDL_free(prefPath);
}

ASEXPORT asResults asUserFileOpen(asUserFile* pUserFile, const char* fileName, size_t fileNameLength, const char* mode)
{
	const size_t fileNameBuffferSize = prefPathLength + fileNameLength + 1;
	char* fileNameBuffer = asMalloc(fileNameBuffferSize);
	ASASSERT(fileNameBuffer);
	memset(fileNameBuffer, 0, fileNameBuffferSize);
	strncat(fileNameBuffer, prefPath, prefPathLength);
	strncat(fileNameBuffer, fileName, fileNameLength);

	pUserFile->pPath = fileNameBuffer;
	pUserFile->pFile = fopen(fileNameBuffer, mode);

	if (!pUserFile->pFile)
	{
		return AS_FAILURE_FILE_INACCESSIBLE;
	}
	return AS_SUCCESS;
}

ASEXPORT size_t asUserFileGetSize(asUserFile* pUserFile)
{
	size_t currentFilePos = ftell(pUserFile->pFile);
	fseek(pUserFile->pFile, 0, SEEK_END);
	size_t size = ftell(pUserFile->pFile);
	fseek(pUserFile->pFile, currentFilePos, SEEK_SET);
	return size;
}

ASEXPORT size_t asUserFileRead(void* pDest, size_t size, size_t count, asUserFile* pUserFile)
{
	return fread(pDest, size, count, pUserFile->pFile);
}

ASEXPORT size_t asUserFileWrite(void* pSrc, size_t size, size_t count, asUserFile* pUserFile)
{
	return fwrite(pSrc, size, count, pUserFile);
}

ASEXPORT void asUserFileClose(asUserFile* pUserFile)
{
	if(pUserFile->pFile)
		fclose(pUserFile->pFile);
	asFree(pUserFile->pPath);
}
