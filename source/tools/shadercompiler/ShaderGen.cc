#include "ShaderGen.h"
#include "FxAssembler.h"

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
#define  STRPOOL_IMPLEMENTATION
#include "mattias/strpool.h"

#include "GLSLGenerator.h"
#include "SPIRVCompiler.h"

/*Returns next line after*/
char* getNextMacro(char* input, char* pNameOut, size_t nameOutSize, char* pParamOut, size_t paramOutSize, bool returnBegin)
{
	char* startPos;
	char* endPos;
	char* paramStartPos;
	char* paramEndPos;
	char* nameStartPos;
	char* nameEndPos;
	memset(pNameOut, 0, nameOutSize);

	/*Find macro start*/
	startPos = strchr(input, '@');
	if (startPos == NULL) {
		return NULL;
	}
	nameStartPos = startPos + 1;
	/*Find macro end*/
	endPos = strchr(startPos, '\r');
	if (!endPos) {
		endPos = strchr(startPos, '\n');
		if (!endPos) {
			endPos = strchr(startPos, 0);
			if (!endPos)
				return NULL;
		}	
	}

	nameEndPos = endPos;
	/*Find param start*/	
	paramStartPos = strchr(startPos, '(');
	if (paramStartPos != NULL && paramStartPos < endPos) {
		paramStartPos += 1;
		paramEndPos = strchr(paramStartPos, ')');
		nameEndPos = paramStartPos - 1;
		memset(pParamOut, 0, paramOutSize);
		if (pParamOut != NULL)
		{
			strncpy_s(pParamOut, paramOutSize, paramStartPos, (paramEndPos - paramStartPos));
			pParamOut[(paramEndPos - paramStartPos)] = '\0';
		}
	}

	/*Return Name*/
	strncpy_s(pNameOut, nameOutSize, nameStartPos, (nameEndPos - nameStartPos));
	pNameOut[(nameEndPos - nameStartPos)] = '\0';

	if (returnBegin)
		return startPos;

	while (endPos[0] == '\n' || endPos[0] == '\r')
	{
		endPos++;
	}
	return endPos;
}

char* retrieveSection(char* input, size_t inputSize,
	const char* startMacro,
	char* params, size_t paramBuffSize,
	char* sectionStorage, size_t sectionBuffSize)
{
	char* parsePos = input;
	char* sectionEnd;
	char nameBuffer[512];
	while (parsePos < input + inputSize)
	{
		parsePos = getNextMacro(parsePos, nameBuffer, 512, params, paramBuffSize, false);
		if (parsePos == NULL)
			return NULL;
		if (strcmp(nameBuffer, startMacro) == 0)
		{
			sectionEnd = getNextMacro(parsePos, nameBuffer, 512, NULL, 0, true);
			if (sectionEnd == NULL)
			{
				asDebugLog("Failed to end Section");
				return NULL;
			}
			if (strcmp(nameBuffer, "end") == 0)
			{
				strncpy_s(sectionStorage, sectionBuffSize, parsePos, (sectionEnd - parsePos));
				sectionStorage[(sectionEnd - parsePos)] = '\0';
				/*asDebugLog("%s\n", sectionStorage);*/
			}
			parsePos = sectionEnd + strlen(nameBuffer);
			return parsePos;
		}
	}
	return NULL;
}

#define TMP_SIZE 10000

int ShaderGenerator::parseShaderFx(cFxContext& fx, char* fileContents, size_t fileSize)
{
	/*Parse the file*/
	std::vector<char> tmp;
	tmp.resize(TMP_SIZE);
	char param[512];
	char* parsePos;

	/*Material*/
	if (!retrieveSection(fileContents, fileSize, "beginMaterial", NULL, 0, tmp.data(), TMP_SIZE))
	{
		return -1;
	}
	fx.fxAssemblerSetupMaterialProps(tmp.data(), TMP_SIZE);
	parsePos = fileContents;

	/*Config*/
	if (!retrieveSection(fileContents, fileSize, "beginConfig", NULL, 0, tmp.data(), TMP_SIZE))
	{
		return -2;
	}
	fx.fxAssemblerSetupFixedFunctionProps(tmp.data(), TMP_SIZE);

	/*GLSL Snippets*/
	parsePos = fileContents;
	while (parsePos < fileContents + fileSize)
	{
		parsePos = retrieveSection(parsePos, fileSize, "beginGLSL", param, 512, tmp.data(), TMP_SIZE);
		if (parsePos == NULL)
			break;
		fx.fxAssemblerAddGeneratorProp(param, strlen(param), tmp.data(), strlen(tmp.data()));
	}
	return 0;
}