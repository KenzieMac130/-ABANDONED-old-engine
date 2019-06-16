#pragma once
#include "engine/asRendererCore.h"
#define STRPOOL_U64 uint64_t
#include "mattias/strpool.h"

#include <vector>
#include <string>
#include <sstream>

class cFxContext
{
public:
	cFxContext(std::ostringstream *debugStream);
	~cFxContext();
	std::ostringstream *pDebugStream;

	void fxAssemblerSetupMaterialProps(char* input, size_t inputSize);
	void fxAssemblerSetupFixedFunctionProps(char* input, size_t inputSize);
	void fxAssemblerAddGeneratorProp(char* name, size_t nameSize, char* prop, size_t propSize);
	void fxAssemblerAddNativeShaderCode(asHash32_t nameHash, asShaderStage stage, const char* code, size_t size);

	int saveToFile(const char* filename);

	asShaderFxDesc_t desc;
	std::vector<std::string> FxPropNames;
	std::vector<asHash64_t> GenNameHashes;
	std::vector<std::string> GenValues;
	
	int defaultBufferPadding;
};