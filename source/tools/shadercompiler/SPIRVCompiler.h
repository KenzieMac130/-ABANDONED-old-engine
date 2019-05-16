#pragma once
#include "GLSLGenerator.h"

typedef struct {
	const char* name;
	const char* value;
} spirvGenMacro;

typedef struct {
	uint32_t macroCount;
	spirvGenMacro* pMacros;
	const char* includePath;
	struct shaderc_compilation_result* result;
	struct shaderc_compiler* compiler;
} spirvGenContext_t;

int genSpirvFromGlsl(spirvGenContext_t* ctx, glslGenContext_t* glsl, asShaderStage stage);
size_t spirvGenContext_GetLength(spirvGenContext_t* ctx);
void* spirvGenContext_GetBytes(spirvGenContext_t* ctx);
void spirvGenContext_Free(spirvGenContext_t* ctx);