#include "FileParser.h"
#include "FxAssembler.h"

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
	startPos = strchr(input, '#');
	if (startPos == NULL) {
		return NULL;
	}
	nameStartPos = startPos + 1;
	/*Find macro end*/
	endPos = strchr(startPos, '\r');
	if (endPos == NULL) {
		endPos = strchr(startPos, '\n');
		return NULL;
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
				exit(-1);
			}
			if (strcmp(nameBuffer, "end") == 0)
			{
				strncpy_s(sectionStorage, sectionBuffSize, parsePos, (sectionEnd - parsePos));
				sectionStorage[(sectionEnd - parsePos)] = '\0';
				asDebugLog("%s\n", sectionStorage);
			}
			parsePos = sectionEnd + strlen(nameBuffer);
			return parsePos;
		}
	}
	return NULL;
}

#define TMP_SIZE 10000

void generateShaderFromFile(char* fileContents, size_t fileSize)
{
	asDebugLog("%s\n", fileContents);
	asDebugLog("===================================================================\n");

	/*Parse the file*/
	char *tmp = malloc(TMP_SIZE);
	char param[512];
	char* parsePos;
	size_t sectionSize;
	fxContext_t* fx = fxAssemblerInit();

	/*Material*/
	parsePos = retrieveSection(fileContents, fileSize, "beginMaterial", NULL, 0, tmp, TMP_SIZE);
	fxAssemblerSetupMaterialProps(fx, fileContents, parsePos - fileContents);

	/*Fixed Functions*/
	retrieveSection(fileContents, fileSize, "beginFixed", NULL, 0, tmp, TMP_SIZE);
	fxAssemblerSetupFixedFunctionProps(fx, fileContents, parsePos - fileContents);

	/*Render Config*/
	retrieveSection(fileContents, fileSize, "beginRenderConfig", NULL, 0, tmp, TMP_SIZE);

	/*Samplers*/
	parsePos = fileContents;
	while (parsePos < fileContents + fileSize)
	{
		parsePos = retrieveSection(parsePos, (fileContents + fileSize)-parsePos, "beginSampler", param, 512, tmp, TMP_SIZE);
		if (parsePos == NULL)
			break;
	}

	/*GLSL Snippets*/
	parsePos = fileContents;
	while (parsePos < fileContents + fileSize)
	{
		parsePos = retrieveSection(parsePos, (fileContents + fileSize) - parsePos, "beginGLSL", param, 512, tmp, TMP_SIZE);
		if (parsePos == NULL)
			break;
	}

	fxAssemblerRelease(fx);
}