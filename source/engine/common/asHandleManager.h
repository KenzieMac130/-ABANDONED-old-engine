#ifndef _ASHANDLEMANAGER_H_
#define _ASHANDLEMANAGER_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

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

#ifdef __cplusplus
}
#endif
#endif