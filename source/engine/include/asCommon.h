#pragma once

#include "astrengineConfig.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

/**
* @file
* @brief Common utilities for the engine
*/

/*Exportable*/
#define ASEXPORT __declspec(dllexport)

/**
* @brief determine the size of a C array
* @warning do not use this for memory created with custom allocators!
*/
#define ASARRAYSIZE(arr) sizeof(arr)/sizeof(arr[0])

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
	const char *pAppName; /**< Name of the app and window */
	asVersion_t appVersion; /**< Version of the app*/
	const char *pGfxIniName; /**< Name of the ini file graphics options should be loaded from */
} asAppInfo_t;

/**
* @brief wraps around printf (but can be overriden in the future to output to remote debug tools)
*/
#define asDebugLog(_format, ...) printf((_format), __VA_ARGS__)
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
ASEXPORT asCfgFile_t* asCfgLoad(const char* path);
/**
* @brief Load config file from memory
* @warning allocates memory on heap, make sure to asCfgFree()
*/
ASEXPORT asCfgFile_t* asCfgFromMem(unsigned char* data, size_t size);
/**
* @brief Free the config file
*/
ASEXPORT void asCfgFree(asCfgFile_t* cfg);
/**
* @brief Attempt to open a section from the config file
*/
ASEXPORT void asCfgOpenSection(asCfgFile_t* cfg, const char* name);
/**
* @brief Read a number from the config file
*/
ASEXPORT double asCfgGetNumber(asCfgFile_t* cfg, const char* name, double fallback);
/**
* @brief Read a string from the config file
* @warning when the config file is freed the string is no longer valid 
*/
ASEXPORT const char* asCfgGetString(asCfgFile_t* cfg, const char* name, const char* fallback);

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

#define AS_ALLOC_LINEAR_MAXCONTEXTS 1
/**
* @brief Initialize central linear memory allocator
* @warning do not touch...
*/
ASEXPORT void asAllocInit_Linear(uint32_t memPerContext);
/**
* @brief Shutdown central linear memory allocator
* @warning do not touch...
*/
ASEXPORT void asAllocShutdown_Linear();
/**
* @brief Allocate a short lived block of memory using the linear allocator
* @warning you should asAlloc_LinearFree() your allocations in a reverse order
* @param arenaIdx use this to change to a different pool for thread-safety
* @warning arenaIdx must not exceed AS_ALLOC_LINEAR_MAXCONTEXTS
*/
ASEXPORT void* asAlloc_LinearMalloc(size_t size, size_t arenaIdx);
/**
* @brief free memory allocated with asAlloc_LinearMalloc()
* @warning the allocations should be freed in reverse order
* @warning if you attempt to free something that hasn't been created with asAlloc_LinearMalloc()
* you are in for a bad time
*/
ASEXPORT void asAlloc_LinearFree(void* block);

/**
* @brief Handle into a handle manager
* @warning you shouldn't direclty modify these values
*/
typedef struct
{
	unsigned _index : 8;
	unsigned _generation : 24;
} asHandle_t;
int asHandle_toInt(asHandle_t hndl);

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

ASEXPORT asHash64_t asHashBytes64_xxHash(const void *pBytes, size_t size);