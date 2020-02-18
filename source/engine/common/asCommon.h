#pragma once

#ifdef __cplusplus 
extern "C" { 
#endif

#include "../astrengineConfig.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>

/**
* @file
* @brief Common utilities for the engine
*/

/**
* @brief Return codes and errors
*/
typedef enum
{
	AS_SUCCESS = 0,
	AS_FAILURE_UNKNOWN = 1,
	AS_FAILURE_OUT_OF_MEMORY = 2,
	AS_FAILURE_INVALID_PARAM = 3,
	AS_FAILURE_UNSUPPORTED_HARDWARE = 4,
	AS_FAILURE_UNKNOWN_FORMAT = 5,
	AS_FAILURE_OUT_OF_BOUNDS = 6,
	AS_FAILURE_PARSE_ERROR = 7,
	AS_FAILURE_DECOMPRESSION_ERROR = 8,
	AS_FAILURE_FILE_NOT_FOUND = 9,
	AS_FAILURE_FILE_INACCESSIBLE = 10
} asResults;

/*Assert*/
#define ASASSERT(e) assert(e)

/*Exportable*/
#define ASEXPORT __declspec(dllexport)

/**
* @brief determine the size of a C array
* @warning do not use this for memory created with custom allocators!
*/
#define ASARRAYLEN(arr) sizeof(arr)/sizeof(arr[0])

typedef struct {
	int major; /**< Major version of the app or game */
	int minor; /**< Minor version of the app or game */
	int patch; /**< Patch number of the app or game */
} asVersion_t;

/**
* @brief Config for starting up astrengine
*
* @note if asIgnite_Config_t::pCustomWindow is null then the engine will attempt to create a new window
* @note astrengine will attempt to load graphics configurations from asIgnite_Config_t::pGfxIniName
* otherwise it will fall back to default settings and the default window size provided in this struct
*/
typedef struct {
	const char *pAppName; /**< Name of the app and window*/
	const char* pDevName; /**< Name of the developer*/
	asVersion_t appVersion; /**< Version of the app*/
} asAppInfo_t;

/**
* @brief wraps around printf (but can be overriden in the future to output to remote debug tools)
*/
#define asDebugLog(_format, ...) printf((_format"\n"), __VA_ARGS__)

/**
* @brief wraps around printf (but can be overriden in the future to output to remote debug tools)
*/
#define asDebugWarning(_format, ...) printf(("[WARNING] " _format"\n"), __VA_ARGS__)
/**
* @brief Display an error in a platform independent way
*/
ASEXPORT void asError(const char* msg);
/**
* @brief A fatal error that should close the engine
*/
ASEXPORT void asFatalError(const char* msg);

/**
* @brief should behave just like malloc
*/
#define asMalloc(size) malloc(size)
/**
* @brief should behave just like realloc
*/
#define asRealloc(block, size) realloc(block, size)
/**
* @brief should behave just like free
*/
#define asFree(block) free(block)

#include "asConfigFile.h"
#include "asLinearMemoryAllocator.h"
#include "asHandleManager.h"
#include "asIndirectionTable.h"
#include "asHashing.h"
#include "asTime.h"

#ifdef __cplusplus
}
#endif