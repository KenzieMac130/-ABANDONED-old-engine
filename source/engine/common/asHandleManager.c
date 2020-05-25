#include "asHandleManager.h"

/*Handle manager*/

asHandle_t _constructHandle(uint32_t index, uint32_t generation)
{
	asHandle_t result;
	result._index = index;
	result._generation = generation;
	return result;
}

ASEXPORT asHandle_t asHandle_Invalidate()
{
	return _constructHandle(0xFFFFFF, 0xFF);
}

ASEXPORT void asHandleManagerCreate(asHandleManager_t *pMan, uint32_t maxSlots)
{
	pMan->_maxSlots = maxSlots;
	pMan->_slotCount = 0;
	pMan->_freeIndicesCount = 0;
	pMan->pGeneration = asMalloc((sizeof(pMan->pGeneration[0]) * maxSlots) +
		(sizeof(pMan->pFreeIndices[0]) * maxSlots));
	pMan->pFreeIndices = (uint32_t*)(pMan->pGeneration + maxSlots);
	memset(pMan->pGeneration, 0, (sizeof(pMan->pGeneration[0]) * maxSlots) +
		(sizeof(pMan->pFreeIndices[0]) * maxSlots));
}
ASEXPORT void asHandleManagerDestroy(asHandleManager_t *pMan)
{
	pMan->_maxSlots = 0;
	asFree(pMan->pGeneration);
}

ASEXPORT asHandle_t asCreateHandle(asHandleManager_t* pMan)
{
	if (pMan->_slotCount >= pMan->_maxSlots)
		return asHandle_Invalidate();
	uint32_t idx;
	if (pMan->_freeIndicesCount > 0)
	{
		idx = pMan->pFreeIndices[0];
		memcpy(pMan->pFreeIndices, pMan->pFreeIndices + 1, (pMan->_freeIndicesCount - 1) * sizeof(pMan->pFreeIndices[0]));
		pMan->_freeIndicesCount--;
		if (pMan->_freeIndicesCount >= pMan->_maxSlots)
			pMan->_freeIndicesCount = 0;
	}
	else
	{
		pMan->pGeneration[pMan->_slotCount] = 0;
		idx = pMan->_slotCount;
		pMan->_slotCount++;
	}
	ASASSERT(pMan->_freeIndicesCount < pMan->_maxSlots);
	return _constructHandle(idx, pMan->pGeneration[idx]);
}

ASEXPORT void asDestroyHandle(asHandleManager_t* pMan, asHandle_t hndl)
{
	if (hndl._index >= pMan->_maxSlots)
		return;
	if (pMan->_freeIndicesCount >= pMan->_maxSlots)
		pMan->_freeIndicesCount = 0;
	++pMan->pGeneration[hndl._index];
	pMan->pFreeIndices[pMan->_freeIndicesCount] = hndl._index;
	pMan->_freeIndicesCount++;
}

ASEXPORT bool asHandleExists(asHandleManager_t* pMan, asHandle_t hndl)
{
	if (hndl._index >= pMan->_maxSlots)
		return false;
	return hndl._generation == pMan->pGeneration[hndl._index];
}

ASEXPORT bool asHandleValid(asHandle_t hndl)
{
	return hndl._index != 0xFFFFFF;
}