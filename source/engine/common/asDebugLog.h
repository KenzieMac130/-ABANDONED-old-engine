#ifndef _ASDEBUGLOG_H_
#define _ASDEBUGLOG_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

typedef enum {
	AS_DEBUGLOG_MESSAGE,
	AS_DEBUGLOG_WARNING,
	AS_DEBUGLOG_ERROR,
	AS_DEBUGLOG_COUNT,
	AS_DEBUGLOG_MAX = UINT32_MAX
} asDebugLogSeverity;

/**
* @brief wraps around printf (but can be overriden in the future to output to remote debug tools)
*/
ASEXPORT void _asDebugLoggerLog(asDebugLogSeverity level, const char* format, ...);
ASEXPORT void _asDebugLoggerLogArgs(asDebugLogSeverity level, const char* format, va_list args);

/**
* @brief treat it as you would printf
*/
#define asDebugLog(_format, ...) _asDebugLoggerLog(AS_DEBUGLOG_MESSAGE, _format, __VA_ARGS__)

/**
* @brief treat it as you would printf (for resolvable errors)
*/
#define asDebugWarning(_format, ...) _asDebugLoggerLog(AS_DEBUGLOG_WARNING, _format, __VA_ARGS__)

/**
* @brief treat it as you would printf (for resolvable errors)
*/
#define asDebugError(_format, ...) _asDebugLoggerLog(AS_DEBUGLOG_ERROR, _format, __VA_ARGS__)

/*Secure the logger for read access (DO NOT TOUCH UNLESS YOU KNOW WHAT YOU ARE DOING)*/
ASEXPORT asResults _asDebugLoggerGetEntrySecureLogger();
ASEXPORT void _asDebugLoggerGetEntryReleaseLogger();

/*Get Debug Log Message Count (DO NOT USE WHILE LOGGING!!!)*/
ASEXPORT size_t _asDebugLoggerGetEntryCount();

/*Get Debug Log Message at Index (Oldest to Newest)*/
ASEXPORT asResults _asDebugLoggerGetEntryAtIdx(size_t idx, asDebugLogSeverity* pLevel, const char** ppString, size_t* pLength);

/*Setup Log Dumper*/
ASEXPORT asResults _asDebugLoggerInitializeFile(const char* path, size_t freq);
ASEXPORT asResults _asDebugLoggerSetSaveFreq(size_t freq);

#ifdef __cplusplus
}
#endif
#endif