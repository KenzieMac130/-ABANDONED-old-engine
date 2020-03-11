#include "asReflectIOBinary.h"

#include "asReflectImpliment.h"

ASEXPORT size_t asReflectGetBinarySize(const asReflectContainer* pReflectData, uint32_t srcCount)
{
	uint32_t actualMemberCount = asReflectContainerMemberCount(pReflectData);
	return sizeof(asReflectContainer) + /*Reflect Data*/
		(sizeof(asReflectEntry) * actualMemberCount) + /*Entries*/
		sizeof(uint32_t) + /*Data Size*/
		(size_t)pReflectData->binarySize * srcCount; /*Data Blob*/
}

ASEXPORT asResults asReflectSaveToBinary(
	unsigned char* pDest,
	const size_t destSize,
	const asReflectContainer* pReflectData,
	const unsigned char* pSrc,
	const uint32_t srcCount)
{
	/*Destination has space?*/
	if (destSize < asReflectGetBinarySize(pReflectData, srcCount))
	{
		return AS_FAILURE_OUT_OF_BOUNDS;
	}

	/*Save Reflection Container*/
	asReflectContainer containerFinal = *pReflectData;
	containerFinal.data = NULL;
	containerFinal.entryCount = asReflectContainerMemberCount(pReflectData);
	memcpy(&pDest[0], &containerFinal, sizeof(asReflectContainer));

	/*Save Reflection Entries*/
	memcpy(&pDest[sizeof(asReflectContainer)], pReflectData->data, sizeof(asReflectEntry) * containerFinal.entryCount);

	/*Save Source Data Count*/
	memcpy(&pDest[sizeof(asReflectContainer) + sizeof(asReflectEntry) * containerFinal.entryCount],
		&srcCount, sizeof(uint32_t));

	/*Save Source Data*/
	uint32_t finalSize = (uint32_t)pReflectData->binarySize * srcCount;
	memcpy(&pDest[sizeof(asReflectContainer) + (sizeof(asReflectEntry) * containerFinal.entryCount) + sizeof(uint32_t)],
		pSrc, finalSize);

	return AS_SUCCESS;
}

ASEXPORT asResults asReflectLoadFromBinary(
	unsigned char* pDest,
	const size_t destSize,
	const uint32_t destCount,
	const asReflectContainer* pReflectData,
	const unsigned char* pSrc,
	const size_t srcSize,
	uint32_t* pSrcCount,
	struct asReflectContainer** ppOutSrcReflectData)
{
	if (!pSrc)
	{
		return AS_FAILURE_INVALID_PARAM;
	}

	/*Load Container*/
	if (srcSize < sizeof(asReflectContainer))
	{
		return AS_FAILURE_OUT_OF_BOUNDS;
	}
	asReflectContainer* pSrcReflectData = &pSrc[0];

	/*Fix Entries Pointer*/
	pSrcReflectData->data = (asReflectEntry*)&pSrc[sizeof(asReflectContainer)];

	/*Get source count*/
	uint32_t srcCount;
	memcpy(&srcCount, &pSrc[sizeof(asReflectContainer) + (sizeof(asReflectEntry) * pSrcReflectData->entryCount)], sizeof(uint32_t));
	if (pSrcCount)
	{
		*pSrcCount = srcCount;
	}

	/*Get Size*/
	const size_t entryCount = asReflectContainerMemberCount(pReflectData);

	/*Reflection Data Bounds Check*/
	if (srcSize < sizeof(asReflectContainer) + (sizeof(asReflectEntry) * pSrcReflectData->entryCount) + sizeof(uint32_t))
	{
		return AS_FAILURE_OUT_OF_BOUNDS;
	}
	if (ppOutSrcReflectData)
	{
		*ppOutSrcReflectData = &pSrcReflectData;
	}

	/*Return if Nothing to Load (just get the count)*/
	if (!pDest)
	{
		return AS_SUCCESS;
	}

	/*Compare Names*/
	if (strncmp(pSrcReflectData->name, pReflectData->name, 48))
	{
		return AS_FAILURE_PARSE_ERROR;
	}

	/*Find base of Source blob*/
	unsigned char* srcBlob = &pSrc[sizeof(asReflectContainer) + (sizeof(asReflectEntry) * pSrcReflectData->entryCount) + sizeof(uint32_t)];

	/*Choose lesser count*/
	const uint32_t countToLoad = srcCount > destCount ? destCount : srcCount;

	/*For each Source Entry*/
	const uint32_t actualDestEntryCount = asReflectContainerMemberCount(pReflectData);
	for (int i = 0; i < entryCount; i++)
	{
		/*For each Destination Entry*/
		for (int j = 0; j < actualDestEntryCount; j++)
		{
			/*Find Match in Destination*/
			if(strncmp(&pSrcReflectData->data[i].typeName, &pReflectData->data[j].typeName, 30) == 0 &&
			strncmp(&pSrcReflectData->data[i].varName, &pReflectData->data[j].varName, 30) == 0)
			{
				/*Chose lesser size*/
				const size_t copySize =
					pSrcReflectData->data[i].dataSize > pReflectData->data[j].dataSize ?
					pReflectData->data[j].dataSize :
					pSrcReflectData->data[i].dataSize;

				/*Load each in array*/
				const size_t srcStride = pSrcReflectData->binarySize;
				const size_t destStride = pSrcReflectData->binarySize;
				const asReflectEntry* srcBuffer = pSrcReflectData->data;
				const asReflectEntry* destBuffer = pReflectData->data;
				for (int k = 0; k < countToLoad; k++)
				{
					memcpy(&pDest[(destStride * k) + destBuffer[j].dataOffset],
						&srcBlob[(srcStride * k) + srcBuffer[i].dataOffset],
						copySize);
				}
				break;
			}
		}
	}

	return AS_SUCCESS;
}
