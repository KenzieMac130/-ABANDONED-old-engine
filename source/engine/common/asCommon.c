#include "asCommon.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SDL_timer.h>

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
#define  STRPOOL_IMPLEMENTATION
#include "mattias/strpool.h"
#define HASHTABLE_IMPLEMENTATION
#include "mattias/hashtable.h"

/*Errors*/

ASEXPORT void asError(const char* msg)
{
#ifdef _WIN32
	MessageBoxA(0, (LPCSTR)msg, (LPCSTR)"astrengine Error", MB_OK);
#endif
}

ASEXPORT void asFatalError(const char* msg)
{
	asError(msg);
	exit(1);
}

/*Config file*/
#define INI_IMPLEMENTATION
#include "../thirdparty/mattias/ini.h"

struct asCfgFile_t
{
	ini_t *pIni;
	int currentSection;
	int nextProperty;
};

ASEXPORT asCfgFile_t* asCfgLoad(const char* path)
{
	if (!path)
		return NULL;
	FILE *fp;
	fopen_s(&fp, path, "rb");
	if (!fp) {
		asDebugLog("Couldn't Open: %s\n", path);
		return NULL;
	}
	asDebugLog("Opened: %s\n", path);
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp)+1;
	fseek(fp, 0, SEEK_SET);
	char* pCfgData = (char*)asMalloc(size);
	fread(pCfgData, 1, size, fp);
	pCfgData[size-1] = '\0';
	fclose(fp);
	asCfgFile_t *result = asMalloc(sizeof(asCfgFile_t));
	result->pIni = ini_load(pCfgData, NULL);
	result->currentSection = INI_GLOBAL_SECTION;
	result->nextProperty = 0;
	asFree(pCfgData);
	return result;
}
ASEXPORT asCfgFile_t* asCfgFromMem(unsigned char* data, size_t size)
{
	asCfgFile_t *result = asMalloc(sizeof(asCfgFile_t));
	result->pIni = ini_load(data, NULL);
	result->currentSection = INI_GLOBAL_SECTION;
	result->nextProperty = 0;
	return result;
}
ASEXPORT void asCfgFree(asCfgFile_t* cfg)
{
	if (!cfg)
		return;
	ini_destroy(cfg->pIni);
	asFree(cfg);
}
ASEXPORT void asCfgOpenSection(asCfgFile_t* cfg, const char* name)
{
	if (!cfg)
		return;
	int section = ini_find_section(cfg->pIni, name, (int)strlen(name));
	if (section == INI_NOT_FOUND)
		return;
	cfg->currentSection = section;
	cfg->nextProperty = 0;
}
ASEXPORT double asCfgGetNumber(asCfgFile_t* cfg, const char* name, double fallback)
{
	if (!cfg)
		return fallback;
	int index = ini_find_property(cfg->pIni, cfg->currentSection, name, (int)strlen(name));
	if (index == INI_NOT_FOUND)
		return fallback;
	const char* propStr = ini_property_value(cfg->pIni, cfg->currentSection, index);
	char* endptr;
	double value = strtod(propStr, &endptr);
	if (endptr)
		return value;
	else
		return fallback;
}
ASEXPORT const char* asCfgGetString(asCfgFile_t* cfg, const char* name, const char* fallback)
{
	if (!cfg)
		return fallback;
	int index = ini_find_property(cfg->pIni, cfg->currentSection, name, (int)strlen(name));
	if (index == INI_NOT_FOUND)
		return fallback;
	const char* propStr = ini_property_value(cfg->pIni, cfg->currentSection, index);
	if (propStr)
		return propStr;
	else
		return fallback;
}

ASEXPORT int asCfgGetNextProp(asCfgFile_t* cfg, const char** name, const char** value)
{
	if (!cfg)
		return 0;
	if (cfg->nextProperty >= ini_property_count(cfg->pIni, cfg->currentSection))
		return 0;
	*value = ini_property_value(cfg->pIni, cfg->currentSection, cfg->nextProperty);
	*name = ini_property_name(cfg->pIni, cfg->currentSection, cfg->nextProperty);
	cfg->nextProperty++;
	return 1;
}

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
	struct linearHeader{
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
	if (pMan->_freeIndicesCount)
	{
		idx = pMan->pFreeIndices[0];
		memcpy(pMan->pFreeIndices, pMan->pFreeIndices + 1, (pMan->_freeIndicesCount - 1) * sizeof(pMan->pFreeIndices[0]));
		--pMan->_freeIndicesCount;
	}
	else
	{
		pMan->pGeneration[pMan->_slotCount] = 0;
		idx = pMan->_slotCount;
		pMan->_slotCount++;
	}
	return _constructHandle(idx, pMan->pGeneration[idx]);
}

ASEXPORT void asDestroyHandle(asHandleManager_t* pMan, asHandle_t hndl)
{
	if (hndl._index >= pMan->_maxSlots)
		return;
	++pMan->pGeneration[hndl._index];
	pMan->pFreeIndices[pMan->_freeIndicesCount] = hndl._index;
	++pMan->_freeIndicesCount;
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

/*Hashing*/
#define XXH_STATIC_LINKING_ONLY
#define XXHASH_IMPLEMENTATION
#include "xxhash/xxHash.h"

ASEXPORT asHash64_t asHashBytes64_xxHash(const void *pBytes, size_t size)
{
	return (asHash64_t)XXH64(pBytes, size, 0);
}

ASEXPORT asHash32_t asHashBytes32_xxHash(const void *pBytes, size_t size)
{
	return (asHash32_t)XXH32(pBytes, size, 0);
}

/*Time*/

ASEXPORT asTimer_t asTimerStart()
{
	asTimer_t result;
	result.freq = SDL_GetPerformanceFrequency();
	result.last = SDL_GetPerformanceCounter();
	return result;
}

ASEXPORT asTimer_t asTimerRestart(asTimer_t prev)
{
	prev.last = SDL_GetPerformanceCounter();
	return prev;
}

ASEXPORT uint64_t asTimerTicksElapsed(asTimer_t timer)
{
	uint64_t now = SDL_GetPerformanceCounter();
	return now - timer.last;
}

ASEXPORT uint64_t asTimerMicroseconds(asTimer_t timer, uint64_t ticks)
{
	return ticks / (timer.freq / 1000000);
}