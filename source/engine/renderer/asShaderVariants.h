#ifndef _ASSHADERVARIANTS_H_
#define _ASSHADERVARIANTS_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "engine/common/asCommon.h"
#include "engine/renderer/asRendererCore.h"

#include "engine/common/asBin.h"
#include "engine/renderer/asRenderFx.h"

/**
* @file
* @brief Define expected shader compilation and pipeline generator variations
*/

/*Todo: Ditch MSVC and use flexible array member*/
#define AS_SHADER_MAX_MACROS 8
#define AS_SHADER_MAX_CODEPATHS 10
#define AS_SHADER_MAX_PIPELINES 8

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
* @brief Must alter a proper graphics pipeline description for target API/Backend
*
* @usage fill out the pipeline description for blend/rasterizer states, inputs as needed.
* retrie reflection data. Don't fill out the shader descriptions, its done automatically.
*
* @param pShaderAsBin asbin to read from
* @param asGfxAPIs target graphics api to generate description for 
* @param pDesc description to output to 
* (cast pointer to description struct for API, shaders are already filled out)
* @param pipelineName name of target pipeline
*/
typedef asResults (*asGfxPipelineFillCb)(
	asBinReader* pShaderAsBin,
	asGfxAPIs api,
	asPipelineType type,
	void* pDesc,
	const char* pipelineName,
	void* pUserData);

typedef struct {
	const char* name;
	asPipelineType type;
	asGfxPipelineFillCb fpCreatePipelineCallback;
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