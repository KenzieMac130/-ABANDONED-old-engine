#include "asDebugLog.h"

#include <SDL_thread.h>
SDL_mutex* logAccessMutex = NULL;

#define AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH 4096
#define AS_MAX_INTERNAL_DEBUG_LOG_HISTORY 256

static size_t logBeforeDump = 1;
#define AS_MAX_INTENRAL_DEBUG_LOG_BEFORE_DUMP logBeforeDump

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
	if (logAccessMutex)
	{
		SDL_UnlockMutex(logAccessMutex);
		SDL_DestroyMutex(logAccessMutex);
		logAccessMutex = NULL;
	}

	if (pLogFile)
	{
		_dumpUpToIndex(nextDumpIndex, writtenSinceLastDump);
		fclose(pLogFile);
	}
}

ASEXPORT asResults _asDebugLoggerGetEntrySecureLogger()
{
	if (SDL_LockMutex(logAccessMutex) == 0) { return AS_SUCCESS; }
	return AS_FAILURE_UNKNOWN;
}

ASEXPORT void _asDebugLoggerGetEntryReleaseLogger()
{
	SDL_UnlockMutex(logAccessMutex);
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

ASEXPORT asResults _asDebugLoggerInitializeFile(const char* path, size_t freq)
{
	if (logAccessMutex && SDL_LockMutex(logAccessMutex) == 0)
	{
		if (pLogFile) /*Log file Already Opened*/
		{
			_dumpUpToIndex(nextDumpIndex, writtenSinceLastDump);
			fclose(pLogFile);
		}
		pLogFile = fopen(path, "w");
		logBeforeDump = freq;
		SDL_UnlockMutex(logAccessMutex);
		if (!pLogFile) { return AS_FAILURE_FILE_INACCESSIBLE; }
		return AS_SUCCESS;
	}
}

ASEXPORT asResults _asDebugLoggerSetSaveFreq(size_t freq)
{
	if (logAccessMutex && SDL_LockMutex(logAccessMutex) == 0)
	{
		logBeforeDump = freq;
		SDL_UnlockMutex(logAccessMutex);
	}
}

ASEXPORT void _asDebugLoggerLogArgs(asDebugLogSeverity level, const char* format, va_list args)
{
	if (!logAccessMutex) /*Assumes first call to debug log happens before thread management system is launched*/
	{
		logAccessMutex = SDL_CreateMutex();
		atexit(_logAtExit);
		ASASSERT(logAccessMutex);
	}

	/*Print to OS Console*/
	vprintf(format, args);
	printf("\n");

	/*Print to Internal Ringbuffer*/
	if (SDL_LockMutex(logAccessMutex) == 0)
	{
		internalMessageLog[nextWriteIndex];

		/*Save to internal log*/
		memset(internalMessageLog[nextWriteIndex % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].text, 0,
			AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH);
		vsnprintf(internalMessageLog[nextWriteIndex % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].text,
			AS_MAX_INTERNAL_DEBUG_LOG_INTENRAL_TEXT_LENGTH, format, args);
		internalMessageLog[nextWriteIndex % AS_MAX_INTERNAL_DEBUG_LOG_HISTORY].logLevel = level;

		nextWriteIndex++;
		writtenSinceLastDump++;
		if (internalEntryCount < AS_MAX_INTERNAL_DEBUG_LOG_HISTORY) { internalEntryCount++; }

		/*Dump File as Necessary*/
		if (writtenSinceLastDump >= AS_MAX_INTENRAL_DEBUG_LOG_BEFORE_DUMP && pLogFile)
		{
			_dumpUpToIndex(nextDumpIndex, writtenSinceLastDump);
		}

		SDL_UnlockMutex(logAccessMutex);
	}
	else { asFatalError("Mutex Lock failed in Debug Log!!!", -1); }
}

ASEXPORT void _asDebugLoggerLog(asDebugLogSeverity level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	_asDebugLoggerLogArgs(level, format, args);
	va_end(args);
}