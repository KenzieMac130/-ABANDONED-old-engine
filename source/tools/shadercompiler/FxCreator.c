#include "FxCreator.h"

#include "engine/common/reflection/asReflectIOBinary.h"

#define SECTION_CAPACITY 512

asResults FxCreator_Create(FxCreator* pCreator, const char* path, const char* shaderType)
{
	memset(pCreator, 0, sizeof(FxCreator));
	asResults results = asBinWriterOpen(&pCreator->_binWriter, "ASFX", path, SECTION_CAPACITY);
	if (results != AS_SUCCESS) { return results; }

	asBinWriterAddSection(&pCreator->_binWriter,
		(asBinSectionIdentifier) {"VARIANT", 0}, shaderType, strlen(shaderType)+1);
}

asResults FxCreator_StartDataSection(FxCreator* pCreator, const char* name)
{
	if (pCreator->_dataSectionCount >= AS_MAX_DATA_SECTIONS)
	{
		return AS_FAILURE_OUT_OF_BOUNDS;
	}
	const asHash32_t nameHash = asHashBytes32_xxHash(name, strlen(name));
	for (size_t i = 0; i < pCreator->_dataSectionCount; i++)
	{
		if (pCreator->_dataSectionHashes[i] == nameHash)
		{
			return AS_FAILURE_DUPLICATE_ENTRY;
		}
	}
	asManualSerialListCreate(&pCreator->_activeSerialList, name, 64, 4096, 1);
	strncat(pCreator->dataSectionName, name, 63);

	pCreator->_dataSectionHashes[pCreator->_dataSectionCount] = nameHash;
	pCreator->_dataSectionCount++;

	return AS_SUCCESS;
}

asResults FxCreator_DataSection_AddEntry(FxCreator* pCreator, const char* typeName, const char* name, void* data, size_t size)
{
	asManualSerialListAddProperty(&pCreator->_activeSerialList, typeName, name, data, size);
	return AS_SUCCESS;
}

asResults FxCreator_AddCodeBlock(FxCreator* pCreator, const char* name, const char* content, size_t contentSize, asShaderStage stage, int quality)
{
	if (pCreator->_codeSectionCount >= AS_MAX_CODE_SECTIONS)
	{
		return AS_FAILURE_OUT_OF_BOUNDS;
	}
	pCreator->_codeSections[pCreator->_codeSectionCount] = (asShaderFxProgramDesc){ 
		.defGroupNameHash = asHashBytes32_xxHash(name, strlen(name)),
		.stage = stage,
		.quality = quality,
		.programSection = pCreator->_codeSectionCount,/*Store Current Code Section Index as the Hash (Collision with low numbers will be highly unlikely)*/
	};

	asBinWriterAddSection(&pCreator->_binWriter, (asBinSectionIdentifier) { "SPIR-V", pCreator->_codeSectionCount }, content, contentSize);

	pCreator->_codeSectionCount++;
	return AS_SUCCESS;
}

void FxCreator_EndDataSection(FxCreator* pCreator)
{
	size_t contentSize = asReflectGetBinarySize(&pCreator->_activeSerialList.reflectContainer, 1);
	unsigned char* content = asMalloc(contentSize);
	ASASSERT(content);
	asReflectSaveToBinary(content, contentSize, &pCreator->_activeSerialList.reflectContainer, pCreator->_activeSerialList.pData, 1);
	asBinWriterAddSection(&pCreator->_binWriter,
		(asBinSectionIdentifier) {"BLOB", asHashBytes32_xxHash(pCreator->dataSectionName, strlen(pCreator->dataSectionName))},
		content, contentSize);
	asManualSerialListFree(&pCreator->_activeSerialList);
}

void FxCreator_Finish(FxCreator* pCreator)
{
	asBinWriterAddSection(&pCreator->_binWriter,
		(asBinSectionIdentifier) {"CODESECT", 0},
		pCreator->_codeSections,
		sizeof(asShaderFxProgramDesc) * pCreator->_codeSectionCount);

	asBinWriterClose(&pCreator->_binWriter);
}
