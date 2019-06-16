#pragma once
#include "GLSLGenerator.h"

#include <vector>
#include <sstream>

typedef struct {
	const char* name;
	const char* value;
} spirvGenMacro_t;

class cSpirvGenContext {
public:
	cSpirvGenContext();
	cSpirvGenContext(std::ostringstream *debugStream, const char* includeDir = NULL);
	~cSpirvGenContext();

	std::ostringstream *pDebugStream;

	std::vector<spirvGenMacro_t> macros;
	const char* includePath;

	int genSpirvFromGlsl(cGlslGenContext* glsl);
	size_t GetLength();
	void* GetBytes();

private:
	struct shaderc_compilation_result* result;
	struct shaderc_compiler* compiler;
};