#pragma once
#include "engine/asCommon.h"

typedef struct
{
	int error;
	size_t contentsSize;
	char* pContents;
} shaderGenResult_t;

shaderGenResult_t generateShaderFxFromTemplates(char* fileContents, size_t fileSize, const char* templatePath, const char* includePath);

void shaderGenResult_Free(shaderGenResult_t* res);