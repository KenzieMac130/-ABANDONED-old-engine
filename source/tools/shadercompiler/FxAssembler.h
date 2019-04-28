#include "engine/asRendererCore.h"

typedef struct fxContext_t fxContext_t;

fxContext_t* fxAssemblerInit();

void fxAssemblerSetupMaterialProps(fxContext_t *ctx, char* input, size_t inputSize);
void fxAssemblerSetupFixedFunctionProps(fxContext_t *ctx, char* input, size_t inputSize);

void fxAssemblerRelease(fxContext_t *ctx);