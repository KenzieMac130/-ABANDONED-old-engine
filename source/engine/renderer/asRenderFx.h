#ifndef _ASRENDERFX_H_
#define _ASRENDERFX_H_

#include "../common/asCommon.h"
#include "../resource/asResource.h"
#include "../common/reflection/asReflectIOBinary.h"
#include "asRendererCore.h"

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
* @brief Describes a shader program
* Technically could be described as series of generic shader properties...
* but it was decided against to avoid overgeneralization and creating a meaningless structure
* Plus if the material system completely patched over any of these values it would not be good...
*/
typedef struct {
	asHash32_t defGroupNameHash; /**< Identifies what define group the compiler took when generating (zprepass/gbuffer/forward/etc)*/
	asShaderStage stage; /**< Stage associated with the shader*/
	int32_t quality; /**< Internal quality level associated with shader (must not affect inputs/outputs interface only instructions)*/
	asHash32_t programSection; /**< asBin Section Containing Shader code/bytecode for the current API (Vulkan: SPIR-V, OpenGL: GLSL, DirectX: HLSL, ect...)*/
} asShaderFxProgramDesc;

#ifdef ASTRENGINE_VK
typedef void* asGfxPipelineHandle; /*Vulkan Impliments as Pointer*/
#endif

#define AS_SHADERFX_VERSION 203

#include "../renderer/asShaderVariants.h"

#ifdef __cplusplus
}
#endif
#endif