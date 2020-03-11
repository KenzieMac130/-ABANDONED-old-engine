#ifndef _ASINDIRECTIONTABLE_H_
#define _ASINDIRECTIONTABLE_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

/**
* @brief Index redirection table
* This is useful in combination with handle managers to map 
* an index to values in resizable containers and do deletions without issue
* probably not exactly performant for things like sorting
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
* @brief Deactivate an index (freeing it?)
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

#ifdef __cplusplus
}
#endif
#endif