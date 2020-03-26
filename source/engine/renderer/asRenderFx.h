#ifndef _ASRENDERFX_H_
#define _ASRENDERFX_H_

#include "engine/common/asCommon.h"
#include "engine/resource/asResource.h"
#include "engine/common/reflection/asReflectIOBinary.h"
#include "engine/renderer/asRendererCore.h"
#include "engine/common/asBin.h"

#ifdef __cplusplus
extern "C" {
#endif 

/*Shader FX*/

/**
* @brief shader stages
*/
typedef enum {
	AS_SHADERSTAGE_VERTEX, /**< Vertex shader*/
	AS_SHADERSTAGE_TESS_CONTROL, /**< Tessellation Hull or Control shader*/
	AS_SHADERSTAGE_TESS_EVALUATION, /**< Tessellation Domain or Evaluation shader*/
	AS_SHADERSTAGE_GEOMETRY, /**< Geometry shader*/
	AS_SHADERSTAGE_FRAGMENT, /**< Fragment shader (sometimes referred to as pixel shaders)*/
	AS_SHADERSTAGE_COMPUTE, /**< Compute shaders*/
	AS_SHADERSTAGE_COUNT,
	AS_SHADERSTAGE_MAX = UINT32_MAX
} asShaderStage;

/**
* @brief pipeline types
*/
typedef enum {
	AS_PIPELINETYPE_GRAPHICS,
	AS_PIPELINETYPE_COMPUTE,
	AS_PIPELINETYPE_COUNT,
	AS_PIPELINETYPE_MAX = UINT32_MAX
} asPipelineType;

/**
* @brief Describes a shader program
* Technically could be described as series of generic shader properties...
* but it was decided against to avoid overgeneralization and creating a meaningless structure
* Plus if the material system completely patched over any of these values it would not be good...
*/
typedef struct {
	asHash32_t defGroupNameHash; /**< Identifies what pipeline the compiler took when generating (zprepass/gbuffer/forward/etc)*/
	asShaderStage stage; /**< Stage associated with the shader*/
	int32_t quality; /**< Internal quality level associated with shader (must not affect inputs/outputs interface only instructions)*/
	asHash32_t programSection; /**< asBin Section Containing Shader code/bytecode for the current API (Vulkan: SPIR-V, OpenGL: GLSL, DirectX: HLSL, ect...)*/
} asShaderFxProgramDesc;

#ifdef ASTRENGINE_VK
typedef void* asPipelineHandle; /*Vulkan Impliments as Pointer*/
#endif

#define AS_SHADERFX_VERSION 203

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
* @brief Must alter a proper graphics pipeline description for target API/Backend and return a handle result
*
* @usage fill out the pipeline description for blend/rasterizer states, inputs as needed.
* retrieve reflection data. Don't fill out the shader descriptions, its already populated.
*
* @param pShaderAsBin asbin to read from
* @param asGfxAPIs target graphics api to generate description for
* @param pDesc description to output to
* (cast pointer to description struct for API, shaders are already filled out)
* @param pipelineName name of target pipeline
* @param pOutPipeline the pipeline handle to write back
* @param pUserData custom data for the callback
*/
typedef asResults(*asGfxPipelineFillCb)(
	asBinReader* pShaderAsBin,
	asGfxAPIs api,
	asPipelineType type,
	void* pDesc,
	const char* pipelineName,
	asPipelineHandle* pOutPipeline,
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

typedef struct {
	const char* shaderTypeName;
	asShaderTypeRegistration* registration;
	asPipelineHandle pipelines[AS_SHADER_MAX_PIPELINES]; /*Must match as defined in variants*/
} asShaderFx;

ASEXPORT asResults asCreateShaderFx(asBinReader* pAsbin, asShaderFx* pShaderfx, asQualityLevel minQuality);

ASEXPORT void asFreeShaderFx(asShaderFx* pShaderfx);

#ifdef __cplusplus
}
#endif
#endif