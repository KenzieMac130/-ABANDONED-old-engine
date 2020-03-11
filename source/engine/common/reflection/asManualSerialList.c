#include "asManualSerialList.h"

ASEXPORT asResults asManualSerialListCreate(asManualSerialList* pSerialList, const char* structName, size_t initialPropCount, size_t dataSize, size_t dataCount)
{
	memset(pSerialList->reflectContainer.name, 0, 48);
	strncat(pSerialList->reflectContainer.name, structName, 47);
	pSerialList->fieldCapacity = initialPropCount;
	pSerialList->dataCount = dataCount;
	pSerialList->dataSize = dataSize;
	pSerialList->reflectContainer.data = asMalloc(sizeof(asReflectEntry) * initialPropCount);
	ASASSERT(pSerialList->reflectContainer.data);
	memset(pSerialList->reflectContainer.data, 0, sizeof(asReflectEntry) * initialPropCount);
	pSerialList->reflectContainer.entryCount = 0;
	pSerialList->reflectContainer.binarySize = 0;

	pSerialList->pData = asMalloc(dataCount * dataSize);
	ASASSERT(pSerialList->pData);
	memset(pSerialList->pData, 0, dataCount * dataSize);

	return AS_SUCCESS;
}

ASEXPORT asResults asManualSerialListAddProperties(asManualSerialList* pSerialList, const char* type, const char* name, unsigned char* pSrc, size_t srcSize, size_t stride, size_t count)
{
	pSerialList->reflectContainer.entryCount++;
	if (pSerialList->fieldCapacity < pSerialList->reflectContainer.entryCount)
	{
		return AS_FAILURE_OUT_OF_MEMORY;
	}
	const uint16_t dstBinOffset = pSerialList->reflectContainer.binarySize;
	pSerialList->reflectContainer.binarySize += srcSize * count;
	if (pSerialList->dataSize * pSerialList->dataCount < pSerialList->reflectContainer.binarySize)
	{
		return AS_FAILURE_OUT_OF_MEMORY;
	}
	if (count > pSerialList->dataCount)
	{
		return AS_FAILURE_OUT_OF_MEMORY;
	}

	/*Assign Entry Data*/
	const size_t entryIdx = pSerialList->reflectContainer.entryCount - 1;
	pSerialList->reflectContainer.data[entryIdx] = (asReflectEntry){
		.dataOffset = dstBinOffset,
		.dataSize = (uint16_t)srcSize,
	};
	strncat(pSerialList->reflectContainer.data[entryIdx].typeName, type, 29);
	strncat(pSerialList->reflectContainer.data[entryIdx].varName, name, 29);

	/*Copy to the Destination*/
	for (int i = 0; i < count; i++)
	{
		memcpy(&pSerialList->pData[i * pSerialList->dataSize] + dstBinOffset, &pSrc[i * stride], srcSize);
	}
	return AS_SUCCESS;
}

ASEXPORT asResults asManualSerialListAddProperty(asManualSerialList* pSerialList, const char* type, const char* name, unsigned char* pSrc, size_t srcSize)
{
	return asManualSerialListAddProperties(pSerialList, type, name, pSrc, srcSize, 0, 1);
}

ASEXPORT asResults asManualSerialListFree(asManualSerialList* pSerialList)
{
	asFree(pSerialList->pData);
	asFree(pSerialList->reflectContainer.data);
	return AS_SUCCESS;
}
