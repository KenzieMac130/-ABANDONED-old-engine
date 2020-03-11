#ifndef _ASLINEARMEMORYALLOCATOR_H_
#define _ASLINEARMEMORYALLOCATOR_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

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

#ifdef __cplusplus
}
#endif
#endif