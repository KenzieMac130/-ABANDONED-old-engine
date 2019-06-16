#pragma once
#include "engine/asCommon.h"

#include <sstream>
#include <vector>

#include "FxAssembler.h"

namespace ShaderGenerator
{
	int parseShaderFx(cFxContext& fx, char* fileContents, size_t fileSize);
};