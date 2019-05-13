#pragma once
#include "engine/asRendererCore.h"
#define STRPOOL_U64 uint64_t
#include "mattias/strpool.h"

typedef struct
{
	asShaderFxDesc_t desc;
	STRPOOL_U64 *pDependencies;
	STRPOOL_U64 *pFxPropNames;
	asHash64_t *pGenNameHashes;
	STRPOOL_U64 *pGenValues;
	strpool_t stringPool;
} fxContext_t;

fxContext_t* fxAssemblerInit();

void fxAssemblerAddDependency(fxContext_t *ctx, char* fileName, size_t nameSize);
void fxAssemblerSetupMaterialProps(fxContext_t *ctx, char* input, size_t inputSize);
void fxAssemblerSetupFixedFunctionProps(fxContext_t *ctx, char* input, size_t inputSize);
void fxAssemblerAddGeneratorProp(fxContext_t *ctx, char* name, size_t nameSize, char* prop, size_t propSize);

void fxAssemblerRelease(fxContext_t *ctx);