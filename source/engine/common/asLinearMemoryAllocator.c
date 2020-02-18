#include "asLinearMemoryAllocator.h"

/*Linear allocator*/
#define ALLOCTYPE_LINEAR 1
ASEXPORT void asAllocInit_Linear(asLinearMemoryAllocator_t* memAlloc, size_t size)
{
	memAlloc->_block = (unsigned char*)asMalloc(size);
	memset(memAlloc->_block, 0, size);
	memAlloc->_size = size;
	memAlloc->_nextBase = 0;
	memAlloc->_type = ALLOCTYPE_LINEAR;
}
ASEXPORT void asAllocShutdown_Linear(asLinearMemoryAllocator_t* memAlloc)
{
	asFree(memAlloc->_block);
	memAlloc->_size = 0;
	memAlloc->_nextBase = 0;
}

ASEXPORT void* asAlloc_LinearMalloc(asLinearMemoryAllocator_t* pMemAlloc, size_t size)
{
	ASASSERT(pMemAlloc->_size >= size + (sizeof(uint32_t) * 2) && pMemAlloc->_type == ALLOCTYPE_LINEAR);
	/*Create alloc header*/
	struct linearHeader {
		uint32_t start;
	} *linearAllocHeader = (struct linearHeader*)&pMemAlloc->_block[pMemAlloc->_nextBase];
	linearAllocHeader->start = pMemAlloc->_nextBase;

	void* result = &pMemAlloc->_block[pMemAlloc->_nextBase + sizeof(struct linearHeader)];

	pMemAlloc->_nextBase += (uint32_t)(size + sizeof(struct linearHeader));
	return result;
}
ASEXPORT void asAlloc_LinearFree(asLinearMemoryAllocator_t* pMemAlloc, void* block)
{
	/*Read alloc header*/
	unsigned char* blockBytes = (unsigned char*)block;
	struct linearHeader {
		uint32_t start;
	} *linearAllocHeader = (struct linearHeader*)(blockBytes - (sizeof(struct linearHeader)));
	/*Rewind the linear allocator*/
	pMemAlloc->_nextBase = linearAllocHeader->start;
}