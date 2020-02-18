#include "asIndirectionTable.h"

/*Indirection table*/

ASEXPORT void asIdxIndirectionTableCreate(asIdxIndirectionTable_t *pTable, uint32_t max)
{
	pTable->pIndices = asMalloc(sizeof(pTable->pIndices[0]) * max);
	pTable->_max = max;
	pTable->_upper = 0;
}

ASEXPORT void asIdxIndirectionTableDestroy(asIdxIndirectionTable_t *pTable)
{
	asFree(pTable->pIndices);
	pTable->_max = 0;
	pTable->_upper = 0;
}

ASEXPORT void asIdxTableSetIdx(asIdxIndirectionTable_t *pTable, uint32_t fixedIdx, uint32_t redirectedIdx)
{
	if (fixedIdx > pTable->_upper)
		pTable->_upper = fixedIdx;
	if (fixedIdx >= pTable->_max)
		return;
	pTable->pIndices[fixedIdx] = redirectedIdx;
}

ASEXPORT uint32_t asIdxTableAt(asIdxIndirectionTable_t *pTable, uint32_t fixedIdx)
{
	if (fixedIdx >= pTable->_max)
		return pTable->_max - 1;
	return pTable->pIndices[fixedIdx];
}

ASEXPORT void asIdxTableDeactivateIdx(asIdxIndirectionTable_t *pTable, uint32_t fixedIdx)
{
	return;
}

ASEXPORT void asIdxTableOffsetAfter(asIdxIndirectionTable_t *pTable, uint32_t start, int32_t offset)
{
	for (uint32_t i = 0; i < pTable->_upper; i++)
	{
		if (pTable->pIndices[i] > start)
			pTable->pIndices[i] += offset;
	}
}

ASEXPORT void asIdxTableSwap(asIdxIndirectionTable_t *pTable, uint32_t idxA, int32_t idxB)
{
	for (uint32_t i = 0; i < pTable->_upper; i++)
	{
		if (pTable->pIndices[i] == idxA)
			pTable->pIndices[i] = idxB;
		else if (pTable->pIndices[i] == idxB)
			pTable->pIndices[i] = idxA;
	}
}