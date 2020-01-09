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
* @brief Endian mode (0x00: little, 0xFF; big)
*/
#define AS_ENDIAN 0x00

/**
* @brief Config for starting up astrengine
*
* @note if asIgnite_Config_t::pCustomWindow is null then the engine will attempt to create a new window
* @note astrengine will attempt to load graphics configurations from asIgnite_Config_t::pGfxIniName
* otherwise it will fall back to default settings and the default window size provided in this struct
*/
typedef struct {
	const char *pAppName; /**< Name of the app and window */
	asVersion_t appVersion; /**< Version of the app*/
	const char *pGfxIniName; /**< Name of the ini file graphics options should be loaded from */
} asAppInfo_t;

/**
* @brief wraps around printf (but can be overriden in the future to output to remote debug tools)
*/
#define asDebugLog(_format, ...) printf((_format), __VA_ARGS__)

/**
* @brief wraps around printf (but can be overriden in the future to output to remote debug tools)
*/
#define asDebugWarning(_format, ...) printf(("WARNING: "_format), __VA_ARGS__)
/**
* @brief Display an error in a platform independent way
*/
ASEXPORT void asError(const char* msg);
/**
* @brief A fatal error that should close the engine
*/
ASEXPORT void asFatalError(const char* msg);

/**
* @brief Handle to read a config file
*/
typedef struct asCfgFile_t asCfgFile_t;
/**
* @brief Load config file from disk
* @warning allocates memory on heap, make sure to asCfgFree()
*/
ASEXPORT asCfgFile_t* asCfgLoad(const char* pPath);
/**
* @brief Load config file from memory
* @warning allocates memory on heap, make sure to asCfgFree()
*/
ASEXPORT asCfgFile_t* asCfgFromMem(unsigned char* pData, size_t size);
/**
* @brief Free the config file
*/
ASEXPORT void asCfgFree(asCfgFile_t* pCfg);
/**
* @brief Attempt to open a section from the config file
*/
ASEXPORT void asCfgOpenSection(asCfgFile_t* pCfg, const char* pName);
/**
* @brief Read a number from the config file
*/
ASEXPORT double asCfgGetNumber(asCfgFile_t* pCfg, const char* pName, double fallback);
/**
* @brief Read a string from the config file
* @warning when the config file is freed the string is no longer valid 
*/
ASEXPORT const char* asCfgGetString(asCfgFile_t* pCfg, const char* pName, const char* pFallback);

/**
* @brief Read the next property from the config file
* @warning when the config file is freed the strings are no longer valid
* returns 0 when no more properties are found in the section
*/
ASEXPORT int asCfgGetNextProp(asCfgFile_t* pCfg, const char** ppName, const char** ppValue);

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

/**
* @brief A custom memory allocator base
*/
typedef struct
{
	unsigned char* _block;
	size_t _size;
	size_t _nextBase;
	int _type;
} asLinearMemoryAllocator_t;

/**
* @brief initialize linear memory allocator
* this is best used for short lived memory blocks
* @warning you must asAlloc_LinearFree() your allocations in a reverse order
*/
ASEXPORT void asAllocInit_Linear(asLinearMemoryAllocator_t* pMemAlloc, size_t size);
/**
* @brief Shutdown a linear memory allocator
*/
ASEXPORT void asAllocShutdown_Linear(asLinearMemoryAllocator_t* pMemAlloc);

/**
* @brief Allocate a block of memory for a linear allocator
*/
ASEXPORT void* asAlloc_LinearMalloc(asLinearMemoryAllocator_t* pMemAlloc, size_t size);
/**
* @brief Free a block of memory for a linear allocator
* @warning you must asAlloc_LinearFree() your allocations in a reverse order
*/
ASEXPORT void asAlloc_LinearFree(asLinearMemoryAllocator_t* pMemAlloc, void* pBlock);

/**
* @brief Handle into a handle manager
* @warning you shouldn't direclty modify these values
*/
typedef struct
{
	unsigned _generation : 8;
	unsigned _index : 24;
} asHandle_t;

ASEXPORT asHandle_t asHandle_Invalidate();

/**
* @brief Handle Manager
* Allows you to manage the lifespan of data in an array (or indirection table) without pointers
* @warning you shouldn't direclty modify these values
*/
typedef struct
{
	uint32_t _maxSlots;
	uint32_t _slotCount;
	uint8_t *pGeneration;
	uint32_t _freeIndicesCount;
	uint32_t *pFreeIndices;
} asHandleManager_t;

/**
* @brief Setup a handle manager
*/
ASEXPORT void asHandleManagerCreate(asHandleManager_t *pMan, uint32_t maxSlots);

/**
* @brief Shutdown a handle manager
*/
ASEXPORT void asHandleManagerDestroy(asHandleManager_t *pMan);

/**
* @brief Create a handle
*/
ASEXPORT asHandle_t asCreateHandle(asHandleManager_t* pMan);

/**
* @brief Destroy a handle
*/
ASEXPORT void asDestroyHandle(asHandleManager_t* pMan, asHandle_t hndl);

/**
* @brief Does a handle exist within a manager
* @warning it is possible that this will return a false positive 
* if you hold onto a handle long after it's destruction
* Although this is an rather cheap opertaion, in fact its O(1)
* I recomend you notify systems that depend on the data being handled of
* it's destruction (possibly queued up) so it can cleanup properly rather 
* than using this last minute.
*/
ASEXPORT bool asHandleExists(asHandleManager_t* pMan, asHandle_t hndl);

/**
* @brief Checks if a handle is valid (not if it exists)
* asCreateHandle() may fail if it's capacity is reached
*/
ASEXPORT bool asHandleValid(asHandle_t hndl);

/**
* @brief Index redirection table
* This is useful in combination with handle managers to map 
* an index to values in resizable containers and use sorting algorithms
* @warning you shouldn't direclty modify these underscored values
*/
typedef struct
{
	uint32_t _max;
	uint32_t _upper;
	uint8_t *pIndices; /**< This is an array of indices to redirect to*/
} asIdxIndirectionTable_t;

/**
* @brief Setup an indirection table
*/
ASEXPORT void asIdxIndirectionTableCreate(asIdxIndirectionTable_t *pTable, uint32_t max);

/**
* @brief Shutdown an indirection table
*/
ASEXPORT void asIdxIndirectionTableDestroy(asIdxIndirectionTable_t *pTable);

/**
* @brief Set (or add) a value int the indirection table
*/
ASEXPORT void asIdxTableSetIdx(asIdxIndirectionTable_t *pTable, uint32_t fixedIdx, uint32_t redirectedIdx);

/**
* @brief Lookup the index and return and indirect index
*/
ASEXPORT uint32_t asIdxTableAt(asIdxIndirectionTable_t *pTable, uint32_t fixedIdx);

/**
* @brief Deactivate an index
*/
ASEXPORT void asIdxTableDeactivateIdx(asIdxIndirectionTable_t *pTable, uint32_t fixedIdx);

/**
* @brief Set collapse values after
* this will itterate through all of the indices and add the offset if its higher than the starting value
* example usages would for this might be:
* insertion: asIdxTableOffsetAfter(&myTable, firstPos+count, count),
* and deletion: asIdxTableOffsetAfter(&myTable, firstPos+count, -count)
*/
ASEXPORT void asIdxTableOffsetAfter(asIdxIndirectionTable_t *pTable, uint32_t start, int32_t offset);

/**
* @brief Swap two indices
* this will itterate through all of the indices and swap them with another index if found
* pretty self explainatory for swapping: asIdxTableSwap(&myTable, a, b)
* this will probably be to slow right now to do large scale sorting since its currently O(n)
*/
ASEXPORT void asIdxTableSwap(asIdxIndirectionTable_t *pTable, uint32_t idxA, int32_t idxB);

/*Hashing*/

/**
* @brief a 64 bit hash type
*/
typedef uint64_t asHash64_t;

/**
* @brief Hash bytes to a asHash64_t using the xxHash library
*/
ASEXPORT asHash64_t asHashBytes64_xxHash(const void *pBytes, size_t size);

/**
* @brief a 32 bit hash type
*/
typedef uint32_t asHash32_t;

/**
* @brief Hash bytes to a asHash32_t using the xxHash library
*/
ASEXPORT asHash32_t asHashBytes32_xxHash(const void *pBytes, size_t size);

/*Time*/

/**
* @brief A timer object
*/
typedef struct {
	uint64_t freq;
	uint64_t last;
} asTimer_t;

/**
* @brief Start the timer
*/
ASEXPORT asTimer_t asTimerStart();
/**
* @brief Restart the timer and return it as a new one
*/
ASEXPORT asTimer_t asTimerRestart(asTimer_t prev);

/**
* @brief Get the last time in platform dependent ticks
*/
ASEXPORT uint64_t asTimerTicksElapsed(asTimer_t timer);

/**
* @brief Get mileseconds that passed based on timer
*/
ASEXPORT uint64_t asTimerMicroseconds(asTimer_t timer, uint64_t ticks);

#ifdef __cplusplus
}
#endif