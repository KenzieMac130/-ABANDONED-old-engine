#include "include/asCommon.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SDL_timer.h>

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
	asFree(pCfgData);
	return result;
}
ASEXPORT asCfgFile_t* asCfgFromMem(unsigned char* data, size_t size)
{
	asCfgFile_t *result = asMalloc(sizeof(asCfgFile_t));
	result->pIni = ini_load(data, NULL);
	result->currentSection = INI_GLOBAL_SECTION;
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

/*Linear allocator*/
/*Todo: Fix limited arenas*/
struct
{
	unsigned char* bytes;
	uint32_t maxMemory;
	uint32_t currentOffset;
} _linearAllocArenas[AS_ALLOC_LINEAR_MAXCONTEXTS];
ASEXPORT void asAllocInit_Linear(uint32_t memPerContext)
{
	for (int i = 0; i < AS_ALLOC_LINEAR_MAXCONTEXTS; i++)
	{
		_linearAllocArenas[i].bytes = (unsigned char*)asMalloc(memPerContext);
		memset(_linearAllocArenas[i].bytes, 0, memPerContext);
		_linearAllocArenas[i].maxMemory = memPerContext;
		_linearAllocArenas[i].currentOffset = 0;
	}
}
ASEXPORT void asAllocShutdown_Linear()
{
	for (int i = 0; i < AS_ALLOC_LINEAR_MAXCONTEXTS; i++)
	{
		asFree(_linearAllocArenas[i].bytes);
		_linearAllocArenas[i].maxMemory = 0;
		_linearAllocArenas[i].currentOffset = 0;
	}
}

ASEXPORT void* asAlloc_LinearMalloc(size_t size, size_t arenaIdx)
{
	ASASSERT(_linearAllocArenas[arenaIdx].maxMemory >= size + (sizeof(uint32_t) * 2));
	/*Create alloc header*/
	struct linearHeader{
		uint32_t start;
		uint8_t arena;
	} *linearAllocHeader = (struct linearHeader*)&_linearAllocArenas[arenaIdx].bytes[_linearAllocArenas[arenaIdx].currentOffset];
	linearAllocHeader->arena = (uint8_t)arenaIdx;
	linearAllocHeader->start = _linearAllocArenas[arenaIdx].currentOffset;

	void* result = &_linearAllocArenas[arenaIdx].bytes[
		_linearAllocArenas[arenaIdx].currentOffset + (sizeof(uint32_t) * 2)];

	_linearAllocArenas[arenaIdx].currentOffset += (uint32_t)(size + sizeof(struct linearHeader));
	return result;
}
ASEXPORT void asAlloc_LinearFree(void* block)
{
	/*Read alloc header*/
	unsigned char* blockBytes = (unsigned char*)block;
	struct linearHeader {
		uint8_t start;
		uint32_t arena;
	} *linearAllocHeader = (struct linearHeader*)(blockBytes - (sizeof(struct linearHeader)));
	/*Rewind the linear allocator*/
	_linearAllocArenas[linearAllocHeader->arena].currentOffset = linearAllocHeader->start;
}

/*Handle manager*/

int asHandle_toInt(asHandle_t hndl)
{
	hndl._index = 2;
	hndl._generation = 255;
	int result = hndl._index << 8;
	result |= hndl._generation;
	return result;
}

asHandle_t _constructHandle(uint32_t index, uint32_t generation)
{
	asHandle_t result;
	result._index = index;
	result._generation = generation;
	return result;
}

#define MIN_FREE_INDICES 1024

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
		return _constructHandle(0xFFFFFF, 0xFF);
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

/*Resource Files*/

ASEXPORT asResourceFileID_t asResourceFileIDFromRelativePath(const char* pPath, size_t size)
{
	ASASSERT(size > 1024);
	char tmpBuff[1024];
	memset(tmpBuff, 0, 1024);
	memcpy(tmpBuff, pPath, size);
	/*Remove Windows slashes*/
	for (size_t i = 0; i < size; i++)
	{
		if (tmpBuff[i] == '\\')
			tmpBuff[i] = '/';
	}
	/*Remove relative start*/
	const char* buffStart = strstr(tmpBuff, "resources/");
	if (!buffStart)
		buffStart = tmpBuff;
	/*Remove beginning slash*/
	if (buffStart[0] == '/')
		buffStart++;
	asHashBytes64_xxHash(buffStart, strlen(buffStart));
}