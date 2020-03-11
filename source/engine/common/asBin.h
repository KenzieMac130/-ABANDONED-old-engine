#ifndef _ASBIN_H_
#define _ASBIN_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "asCommon.h"

typedef struct {
	char nameTag[8];
	int64_t index;
} asBinSectionIdentifier;

typedef struct {
	uint64_t offset;
	uint64_t size;
	int32_t flags;
} asBinSectionContent;

typedef struct {
	size_t _sectionCapacity;
	size_t _sectionCount;
	void* _inMemoryList;
	asBinSectionIdentifier* _pSectionIdentifiers;
	asBinSectionContent* _pSections;
	size_t _currentOffset;
	FILE* _fpOut;
	char _fileTag[4];
} asBinWriter;

ASEXPORT asResults asBinWriterOpen(asBinWriter* pWriter, char fileTag[4], const char* path, size_t sectionCapacity);

ASEXPORT asResults asBinWriterAddSection(asBinWriter* pWriter, asBinSectionIdentifier identifier, unsigned char* pData, size_t size);

ASEXPORT asResults asBinWriterClose(asBinWriter* pWriter);

typedef struct {
	size_t sectionCount;
	asBinSectionIdentifier* pSectionIdentifiers;
	asBinSectionContent* pSections;
	unsigned char* pBasePtr;
} asBinReader;

ASEXPORT asResults asBinReaderOpenMemory(asBinReader* pReader, char fileTag[4], unsigned char* pData, size_t size);

ASEXPORT asResults asBinReaderGetSection(asBinReader* pReader, asBinSectionIdentifier identifier, unsigned char** ppData, size_t* pSize);

#ifdef __cplusplus
}
#endif
#endif