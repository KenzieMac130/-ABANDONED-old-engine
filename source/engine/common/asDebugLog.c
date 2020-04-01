#include "asDebugLog.h"

#include <SDL_thread.h>
SDL_mutex* accessMutex = NULL;

#define AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH 4096
#define AS_MAX_INTERNAL_DEBUG_LOG_HISTORY 256

#ifndef NDEBUG
#define AS_MAX_INTENRAL_DEBUG_LOG_BEFORE_DUMP 1
#else
#define AS_MAX_INTENRAL_DEBUG_LOG_BEFORE_DUMP 16
#endif

static size_t internalEntryCount = 0;
static size_t nextDumpIndex = 0;
static size_t nextWriteIndex = 0;
static size_t writtenSinceLastDump = 0;

FILE* pLogFile = NULL;

struct internalMessageContent
{
	asDebugLogSeverity logLevel;
	char text[AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH];
};

static struct internalMessageContent internalMessageLog[AS_MAX_INTERNAL_DEBUG_LOG_HISTORY];

void _dumpUpToIndex(size_t start, size_t count)
{
	for (size_t i = start; i < start + count; i++)
	{
		const char* typeMessage = "[Log]>";
		switch (internalMessageLog[i % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].logLevel)
		{
		case AS_DEBUGLOG_WARNING: typeMessage = "[!WARNING!]>"; break;
		case AS_DEBUGLOG_ERROR: typeMessage = "[!!!ERROR!!!]>"; break;
		}

		fprintf(pLogFile, "%s%.*s\n",
			typeMessage,
			AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH,
			internalMessageLog[i % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].text);
	}
	fflush(pLogFile);
	nextDumpIndex = start + count;
	writtenSinceLastDump = 0;
}

void _logAtExit(void)
{
	if (accessMutex)
	{
		SDL_DestroyMutex(accessMutex);
		accessMutex = NULL;
	}

	if (pLogFile)
	{
		_dumpUpToIndex(nextDumpIndex, writtenSinceLastDump);
		fclose(pLogFile);
	}
}

ASEXPORT asResults _asDebugLoggerGetEntrySecureLogger()
{
	if (SDL_LockMutex(accessMutex) == 0) { return AS_SUCCESS; }
	return AS_FAILURE_UNKNOWN;
}

ASEXPORT void _asDebugLoggerGetEntryReleaseLogger()
{
	SDL_UnlockMutex(accessMutex);
}

ASEXPORT size_t _asDebugLoggerGetEntryCount()
{
	return internalEntryCount;
}

ASEXPORT asResults _asDebugLoggerGetEntryAtIdx(size_t idx, asDebugLogSeverity* pLevel, const char** ppString, size_t* pLength)
{
	if (idx >= internalEntryCount) { return AS_FAILURE_OUT_OF_BOUNDS; }
	size_t trueIdx = ((nextWriteIndex - internalEntryCount) + idx) % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY;

	if (pLevel) { *pLevel = internalMessageLog[trueIdx].logLevel; }
	if (ppString) { *ppString = internalMessageLog[trueIdx].text; }
	if (pLength) { *pLength = strnlen(internalMessageLog[trueIdx].text, AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH); }
	return AS_SUCCESS;
}

ASEXPORT asResults _asDebugLoggerInitializeFile(const char* path)
{
	if (accessMutex && SDL_LockMutex(accessMutex) == 0)
	{
		if (pLogFile) /*Log file Already Opened*/
		{
			_dumpUpToIndex(nextDumpIndex, writtenSinceLastDump);
			fclose(pLogFile);
		}
		pLogFile = fopen(path, "w");
		if (!pLogFile) { return AS_FAILURE_FILE_INACCESSIBLE; }
		return AS_SUCCESS;
	}
}

ASEXPORT void _asDebugLoggerLog(asDebugLogSeverity level, const char* format, ...)
{
	if (!accessMutex) /*Assumes first call to debug log happens before thread management system is launched*/
	{
		accessMutex = SDL_CreateMutex();
		atexit(_logAtExit);
		ASASSERT(accessMutex);
	}

	/*Print to OS Console*/
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");

	/*Print to Internal Ringbuffer*/
	if (SDL_LockMutex(accessMutex) == 0)
	{
		internalMessageLog[nextWriteIndex];

		/*Save to internal log*/
		memset(internalMessageLog[nextWriteIndex % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].text, 0,
			AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH);
		va_list args;
		va_start(args, format);
		vsnprintf(internalMessageLog[nextWriteIndex % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].text,
			AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH, format, args);
		va_end(args);
		internalMessageLog[nextWriteIndex % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].logLevel = level;

		nextWriteIndex++;
		writtenSinceLastDump++;
		if (internalEntryCount < AS_MAX_INTERNAL_DEBUG_LOG_HISTORY) { internalEntryCount++; }

		/*Dump File as Necessary*/
		if (writtenSinceLastDump >= AS_MAX_INTENRAL_DEBUG_LOG_BEFORE_DUMP && pLogFile)
		{
			_dumpUpToIndex(nextDumpIndex, writtenSinceLastDump);
		}

		SDL_UnlockMutex(accessMutex);
	}
	else { asFatalError("Mutex Lock failed in Debug Log!!!"); }
}