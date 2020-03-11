#pragma once

#include "engine/renderer/asShaderVariants.h"
#include "FxCreator.h"

asResults glslToSpirv(FxCreator* pFxCreator, const char* contents, size_t contentsSize, const char* fileName, const char* pipelineName, const asShaderTypeCodePath* codePath);