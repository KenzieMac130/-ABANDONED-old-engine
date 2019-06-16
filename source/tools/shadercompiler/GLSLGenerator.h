#pragma once
#include "FxAssembler.h"

#include <sstream>
#include "engine/asRendererCore.h"

class cGlslGenContext {
public:
	cGlslGenContext(std::ostringstream *debugStream);
	std::ostringstream *pDebugStream;
	asShaderStage stage;
	std::string text;

	void loadGLSLFromFile(const char* filePath);
	void generateGLSLFxFromTemplate(class cFxContext* fx, cGlslGenContext* templateGlsl);
};