#ifndef _ASSHADERVARIANTS_H_
#define _ASSHADERVARIANTS_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "engine/common/asCommon.h"
#include "engine/renderer/asRendererCore.h"

#include "engine/common/asBin.h"

/**
* @file
* @brief Define expected shader compilation and pipeline generator variations
*/

/*Todo: Ditch MSVC and use flexible array member*/
#define AS_SHADER_MAX_MACROS 8
#define AS_SHADER_MAX_CODEPATHS 16
#define AS_SHADER_MAX_PIPELINES 12

typedef struct {
	const char* name;
	const char* value;
} asShaderTypeMacroPair;

typedef struct {
	const char* entry;
	asShaderStage stage;
	asQualityLevel minQuality;
	size_t macroCount;
	asShaderTypeMacroPair macros[AS_SHADER_MAX_MACROS];
} asShaderTypeCodePath;

/**
* @brief Must return a proper graphics pipeline handle for target API/Backend
*/
typedef asGfxPipelineHandle(*asGfxPipelineCreateCb)(
	asBinReader* pShaderAsBin,
	asShaderTypeCodePath* pCodePaths,
	size_t codePathCount,
	const char* pipelineName,
	void* pUserData);

typedef struct {
	const char* name;
	asGfxPipelineCreateCb fpCreatePipelineCallback;
	void* pUserData;
	size_t codePathCount;
	asShaderTypeCodePath codePaths[AS_SHADER_MAX_CODEPATHS];
} asShaderTypePipelineVar;

typedef struct {
	const char* name;
	size_t pipelineCount;
	asShaderTypePipelineVar pipelines[AS_SHADER_MAX_PIPELINES];
} asShaderTypeRegistration;

ASEXPORT const asShaderTypeRegistration* asShaderFindTypeRegistrationByName(const char* name);

#ifdef __cplusplus
}
#endif
#endif