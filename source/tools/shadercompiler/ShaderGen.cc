#include "ShaderGen.h"
#include "FxAssembler.h"

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
#define  STRPOOL_IMPLEMENTATION
#include "mattias/strpool.h"
#define CUTE_FILES_IMPLEMENTATION
#include "cute/cute_files.h"

#include "GLSLGenerator.h"

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

void generateShaderFromFxTemplates(char* fileContents, size_t fileSize, const char* templatePath)
{
	asDebugLog("%s\n\n", fileContents);

	/*Parse the file*/
	char *tmp = (char*)malloc(TMP_SIZE);
	char param[512];
	char nameBuffer[512];
	char* parsePos;
	fxContext_t* fx = fxAssemblerInit();

	/*Material*/
	retrieveSection(fileContents, fileSize, "beginMaterial", NULL, 0, tmp, TMP_SIZE);
	fxAssemblerSetupMaterialProps(fx, tmp, TMP_SIZE);
	parsePos = fileContents;

	/*Fixed Functions*/
	retrieveSection(fileContents, fileSize, "beginFixed", NULL, 0, tmp, TMP_SIZE);
	fxAssemblerSetupFixedFunctionProps(fx, tmp, TMP_SIZE);

	/*Render Config*/
	retrieveSection(fileContents, fileSize, "beginRenderConfig", NULL, 0, tmp, TMP_SIZE);

	/*GLSL Includes*/
	parsePos = fileContents;
	while (parsePos)
	{
		parsePos = getNextMacro(parsePos, nameBuffer, 512, param, 512, false);
		if (strcmp(nameBuffer, "includeGLSL") == 0)
		{
			fxAssemblerAddDependency(fx, param, strlen(param));
		}
	}

	/*GLSL Snippets*/
	parsePos = fileContents;
	while (parsePos < fileContents + fileSize)
	{
		parsePos = retrieveSection(parsePos, fileSize, "beginGLSL", param, 512, tmp, TMP_SIZE);
		if (parsePos == NULL)
			break;
		fxAssemblerAddGeneratorProp(fx, param, strlen(param), tmp, strlen(tmp));
	}

	/*Load Templates*/
	size_t glslVertTemplateCount = 0;
	glslGenContext_t* glslVertTemplates = NULL;
	size_t glslFragTemplateCount = 0;
	glslGenContext_t* glslFragTemplates = NULL;
	{
		cf_dir_t dir;
		if (!cf_file_exists(templatePath))
		{
			asDebugLog("ERROR: TEMPLATE FOLDER \"%s\" INVALID!\n", templatePath);
			return;
		}
		cf_dir_open(&dir, templatePath);
		while (dir.has_next)
		{
			cf_file_t file;
			cf_read_file(&dir, &file);
			asDebugLog("File: %s\n", file.name);
			if (file.is_dir)
			{
				cf_dir_next(&dir);
				continue;
			}
			if (strcmp(file.ext, ".vert") == 0)
			{
				glslGenContext_t glslTemp = glslGenContext_t();
				loadGLSLFromFile(&glslTemp, file.path);
				arrput(glslVertTemplates, glslTemp);
				glslVertTemplateCount++;
			}
			if (strcmp(file.ext, ".frag") == 0)
			{
				glslGenContext_t glslTemp = glslGenContext_t();
				loadGLSLFromFile(&glslTemp, file.path);
				arrput(glslFragTemplates, glslTemp);
				glslFragTemplateCount++;
			}
			cf_dir_next(&dir);
		}
		cf_dir_close(&dir);
	}

	/*Create Permutations*/
	{
		for (int i = 0; i < glslVertTemplateCount; i++)
		{
			glslGenContext_t glsl = glslGenContext_t();
			generateGLSLFxFromTemplate(&glsl, fx, &glslVertTemplates[i]);
		}
	}

	/*Cleanup*/
	for (int i = 0; i < glslVertTemplateCount; i++)
		glslGenContext_Free(&glslVertTemplates[i]);
	arrfree(glslVertTemplates);
	for (int i = 0; i < glslFragTemplateCount; i++)
		glslGenContext_Free(&glslFragTemplates[i]);
	arrfree(glslFragTemplates);
	fxAssemblerRelease(fx);
}