#ifndef _ASUSERFILES_H_
#define _ASUSERFILES_H_

#include "../common/asCommon.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	FILE* pFile;
	const char* pPath;
} asUserFile;

ASEXPORT void asUserFileMakePath(const char* currentName, char* outputBuff, size_t outputBuffSize);

ASEXPORT void asInitUserFiles(const char* developerName, const char* appName);

ASEXPORT void asShutdownUserFiles();

ASEXPORT asResults asUserFileOpen(asUserFile* pUserFile, const char* fileName, size_t fileNameLength, const char* mode);

ASEXPORT size_t asUserFileGetSize(const asUserFile* pUserFile);

ASEXPORT size_t asUserFileRead(void* pDest, size_t size, size_t count, const asUserFile* pUserFile);

ASEXPORT size_t asUserFileWrite(void* pSrc, size_t size, size_t count, const asUserFile* pUserFile);

ASEXPORT void asUserFileClose(asUserFile* pUserFile);

#ifdef __cplusplus
}
#endif
#endif