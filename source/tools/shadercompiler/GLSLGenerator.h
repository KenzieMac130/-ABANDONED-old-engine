#pragma once
#include "FxAssembler.h"

typedef struct {
	size_t size;
	char* byteArray;
} glslGenContext_t;

void loadGLSLFromFile(glslGenContext_t* ctx, const char* filePath);
void generateGLSLFxFromTemplate(glslGenContext_t* ctx, fxContext_t* fx, glslGenContext_t* templateGlsl);
void glslGenContext_Free(glslGenContext_t* ctx);