#pragma once

#include "engine/common/reflection/asManualSerialList.h"
#include "engine/common/asBin.h"
#include "engine/renderer/asRenderFx.h"

#define AS_MAX_DATA_SECTIONS 128
#define AS_MAX_CODE_SECTIONS 64

typedef struct {
	asBinWriter _binWriter;
	asManualSerialList _activeSerialList;
	size_t _dataSectionCount;
	size_t _codeSectionCount;
	char dataSectionName[64];
	asHash32_t _dataSectionHashes[AS_MAX_DATA_SECTIONS];
	asShaderFxProgramDesc _codeSections[AS_MAX_CODE_SECTIONS];
} FxCreator;

asResults FxCreator_Create(FxCreator* pCreator, const char* path);

asResults FxCreator_StartDataSection(FxCreator* creator, const char* name);

asResults FxCreator_DataSection_AddEntry(FxCreator* pCreator, const char* typeName, const char* name, void* data, size_t size);

asResults FxCreator_AddCodeBlock(FxCreator* pCreator, const char* name, const char* content, size_t contentSize, asShaderStage stage, int quality);

void FxCreator_EndDataSection(FxCreator* pCreator);

void FxCreator_Finish(FxCreator* pCreator);