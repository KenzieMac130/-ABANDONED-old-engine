#include "asBin.h"

#define ASBINTAG "ASB1"

struct ASBINHEADER
{
	char asbinTag[4];
	char subtypeTag[4];
	uint64_t sectionCount;
	uint64_t sectionStart;
};

ASEXPORT asResults asBinWriterOpen(asBinWriter* pWriter, char fileTag[4], const char* path, size_t sectionCapacity)
{
	ASASSERT(pWriter);
	ASASSERT(path);
	/*Open File to Save*/
	pWriter->_fpOut = fopen(path, "wb");
	if (!pWriter->_fpOut)
	{
		return AS_FAILURE_FILE_INACCESSIBLE;
	}

	/*Create Section List*/
	const size_t memListSize = (sizeof(asBinSectionContent) + sizeof(asHash32_t)) * sectionCapacity;
	pWriter->_inMemoryList = asMalloc(memListSize);
	ASASSERT(pWriter->_inMemoryList);
	memset(pWriter->_inMemoryList, 0, memListSize);
	pWriter->_sectionCapacity = sectionCapacity;

	/*Fill out Writer*/
	pWriter->_sectionCount = 0;
	strncpy(pWriter->_fileTag, fileTag, 4);
	pWriter->_pSectionIdentifiers = pWriter->_inMemoryList;
	pWriter->_pSections = (void*)&pWriter->_pSectionIdentifiers[sectionCapacity];
	pWriter->_currentOffset = 0;

	/*Make Room for Header*/
	fseek(pWriter->_fpOut, sizeof(struct ASBINHEADER), SEEK_SET);

	return AS_SUCCESS;
}

ASEXPORT asResults asBinWriterAddSection(asBinWriter* pWriter, asBinSectionIdentifier identifier, unsigned char* pData, size_t size)
{
	if (pWriter->_sectionCount >= pWriter->_sectionCapacity)
	{
		return AS_FAILURE_OUT_OF_BOUNDS;
	}

	/*Add Section*/
	pWriter->_pSectionIdentifiers[pWriter->_sectionCount] = identifier;
	pWriter->_pSections[pWriter->_sectionCount] = (asBinSectionContent){
		.offset = pWriter->_currentOffset,
		.size = size,
		.flags = 0 
	};
	pWriter->_sectionCount++;

	/*Write Content*/
	fwrite(pData, size, 1, pWriter->_fpOut);
	pWriter->_currentOffset += size;

	return AS_SUCCESS;
}

ASEXPORT asResults asBinWriterClose(asBinWriter* pWriter)
{
	ASASSERT(pWriter);
	if (!pWriter->_fpOut)
	{
		return AS_FAILURE_FILE_INACCESSIBLE;
	}

	/*Write Sections*/
	fwrite(pWriter->_pSectionIdentifiers, sizeof(asBinSectionIdentifier) * pWriter->_sectionCount, 1, pWriter->_fpOut);
	fwrite(pWriter->_pSections, sizeof(asBinSectionContent) * pWriter->_sectionCount, 1, pWriter->_fpOut);

	/*Write Beginning Content*/
	fseek(pWriter->_fpOut, 0, SEEK_SET);
	struct ASBINHEADER header = (struct ASBINHEADER){
		.asbinTag = ASBINTAG,
		.sectionCount = pWriter->_sectionCount,
		.sectionStart = sizeof(struct ASBINHEADER)
		+ pWriter->_currentOffset
	};
	strncpy(header.subtypeTag, pWriter->_fileTag, 4);
	fwrite(&header, sizeof(struct ASBINHEADER), 1, pWriter->_fpOut);

	fclose(pWriter->_fpOut);
	asFree(pWriter->_inMemoryList);
	return AS_SUCCESS;
}

ASEXPORT asResults asBinReaderOpenMemory(asBinReader* pReader, char fileTag[4], unsigned char* pData, size_t size)
{
	if (size < sizeof(struct ASBINHEADER))
	{
		pReader->sectionCount = 0;
		return AS_FAILURE_OUT_OF_BOUNDS;
	}
	struct ASBINHEADER header;
	header = *(struct ASBINHEADER*)pData;
	if (strncmp(header.asbinTag, ASBINTAG, 4))
	{
		pReader->sectionCount = 0;
		return AS_FAILURE_UNKNOWN_FORMAT;
	}
	if (strncmp(header.subtypeTag, fileTag, 4))
	{
		pReader->sectionCount = 0;
		return AS_FAILURE_UNKNOWN_FORMAT;
	}

	/*Set Pointer Offsets*/
	pReader->sectionCount = header.sectionCount;
	pReader->pBasePtr = (void*)&pData[sizeof(struct ASBINHEADER)];
	pReader->pSectionIdentifiers = (asBinSectionIdentifier*)&pData[header.sectionStart];
	pReader->pSections = (asBinSectionContent*)&pReader->pSectionIdentifiers[header.sectionCount];

	return AS_SUCCESS;
}

ASEXPORT asResults asBinReaderGetSection(asBinReader* pReader, asBinSectionIdentifier identifier, unsigned char** ppData, size_t* pSize)
{
	for (int i = 0; i < pReader->sectionCount; i++)
	{
		if (memcmp(&pReader->pSectionIdentifiers[i], &identifier, sizeof(asBinSectionIdentifier)) == 0)
		{
			if (pSize) {
				*pSize = pReader->pSections[i].size;
			}
			if (ppData) {
				*ppData = &pReader->pBasePtr[pReader->pSections[i].offset];
			}
			return AS_SUCCESS;
		}
	}
	return AS_FAILURE_DATA_DOES_NOT_EXIST;
}
