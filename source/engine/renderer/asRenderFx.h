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

#include "engine/renderer/asShaderVariants.h"

typedef struct {
	const char* shaderTypeName;
	asPipelineHandle pipelines[AS_SHADER_MAX_PIPELINES]; /*Must match as defined in variants*/
} asShaderFx;

ASEXPORT asResults asCreateShaderFx(asBinReader* pAsbin, asShaderFx* pShaderfx, asQualityLevel quality);

ASEXPORT void asFreeShaderFx(asShaderFx* pShaderfx);

#ifdef __cplusplus
}
#endif
#endif