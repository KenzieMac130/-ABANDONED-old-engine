#include "ShaderGen.h"
#include "FxAssembler.h"

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
#define  STRPOOL_IMPLEMENTATION
#include "mattias/strpool.h"
#define CUTE_FILES_IMPLEMENTATION
#include "cute/cute_files.h"

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

shaderGenResult_t generateShaderFxFromTemplates(char* fileContents, size_t fileSize, const char* templatePath, const char* includePath)
{
	shaderGenResult_t result = (shaderGenResult_t) { 0 };
	asTimer_t timer = asTimerStart();
	/*asDebugLog("Loaded Template Contents:\n%s\n\n", fileContents);*/

	/*Parse the file*/
	char *tmp = (char*)malloc(TMP_SIZE);
	char param[512];
	char nameBuffer[512];
	char* parsePos;
	fxContext_t* fx = fxAssemblerInit();

	/*Material*/
	if (!retrieveSection(fileContents, fileSize, "beginMaterial", NULL, 0, tmp, TMP_SIZE))
	{
		fxAssemblerRelease(fx);
		free(tmp);
		result.error = -1;
		return result;
	}
	fxAssemblerSetupMaterialProps(fx, tmp, TMP_SIZE);
	parsePos = fileContents;

	/*Config*/
	if (!retrieveSection(fileContents, fileSize, "beginConfig", NULL, 0, tmp, TMP_SIZE))
	{
		fxAssemblerRelease(fx);
		free(tmp);
		result.error = -2;
		return result;
	}
	fxAssemblerSetupFixedFunctionProps(fx, tmp, TMP_SIZE);

	/*GLSL Snippets*/
	parsePos = fileContents;
	while (parsePos < fileContents + fileSize)
	{
		parsePos = retrieveSection(parsePos, fileSize, "beginGLSL", param, 512, tmp, TMP_SIZE);
		if (parsePos == NULL)
			break;
		fxAssemblerAddGeneratorProp(fx, param, strlen(param), tmp, strlen(tmp));
	}

	asDebugLog("Parsed FX File Microseconds: %"PRIu64"\n", asTimerMicroseconds(timer, asTimerTicksElapsed(timer)));
	timer = asTimerRestart(timer);

	size_t glslTemplateCount = 0;
	glslGenContext_t* glslTemplates = NULL;
	asHash32_t *glslTemplateNames = NULL;
	arrsetcap(glslTemplates, 16);
	arrsetcap(glslTemplateNames, 16);
	/*Load Templates*/
	{
		cf_dir_t dir;
		if (!cf_file_exists(templatePath))
		{
			asDebugLog("ERROR: TEMPLATE FOLDER \"%s\" INVALID!\n", templatePath);
			{
				fxAssemblerRelease(fx);
				free(tmp);
				result.error = -3;
				return result;
			}
		}
		cf_dir_open(&dir, templatePath);
		while (dir.has_next)
		{
			asShaderStage stage;
			cf_file_t file;
			cf_read_file(&dir, &file);
			if (file.is_dir)
			{
				cf_dir_next(&dir);
				continue;
			}
			if (strcmp(file.ext, ".vert") == 0){
				stage = AS_SHADERSTAGE_VERTEX;
			}
			else if (strcmp(file.ext, ".frag") == 0){
				stage = AS_SHADERSTAGE_FRAGMENT;
			}
			else{
				continue;
			}
			asDebugLog("Found Template: %s\n", file.name);
			arrput(glslTemplateNames, asHashBytes32_xxHash(file.name, strlen(file.name)));
			glslGenContext_t glslTemp = (glslGenContext_t) { 0 };
			glslTemp.stage = stage;
			loadGLSLFromFile(&glslTemp, file.path);
			arrput(glslTemplates, glslTemp);
			glslTemplateCount++;
			cf_dir_next(&dir);
		}
		cf_dir_close(&dir);
	}
	asDebugLog("Loaded Templates Microseconds: %"PRIu64"\n", asTimerMicroseconds(timer, asTimerTicksElapsed(timer)));
	timer = asTimerRestart(timer);

	/*Create Programs*/
	asShaderFxProgramDesc_t *glslShaderDescs = NULL;
	{
		arrsetlen(glslShaderDescs, glslTemplateCount);
		uint32_t offset = 0;
		for (int i = 0; i < glslTemplateCount; i++)
		{
			/*Generate GLSL*/
			glslGenContext_t glsl = (glslGenContext_t) { 0 };
			generateGLSLFxFromTemplate(&glsl, fx, &glslTemplates[i]);
			asDebugLog("Generated GLSL Code Microseconds: %"PRIu64"\n", asTimerMicroseconds(timer, asTimerTicksElapsed(timer)));
			timer = asTimerRestart(timer);

			/*Compile SPIRV*/
			spirvGenContext_t spirv = (spirvGenContext_t) { 0 };
			spirv.includePath = includePath;
			spirv.macroCount = 0;
			spirv.pMacros = NULL;
			if (genSpirvFromGlsl(&spirv, &glsl, glslTemplates[i].stage))
				continue; /*SPIR-V Compile Failed*/
			asDebugLog("Compiled SPIRV Microseconds: %"PRIu64"\n", asTimerMicroseconds(timer, asTimerTicksElapsed(timer)));
			timer = asTimerRestart(timer);

			/*Add Code to FX*/
			uint32_t size = spirvGenContext_GetLength(&spirv);
			glslShaderDescs[i].stage = glslTemplates[i].stage;
			glslShaderDescs[i].programByteCount = size;
			glslShaderDescs[i].programByteStart = offset;
			offset += size;
			fxAssemblerAddNativeShaderCode(fx, glslTemplateNames[i],
				spirvGenContext_GetBytes(&spirv), glslShaderDescs[i].programByteCount);

			glslGenContext_Free(&glsl);
			spirvGenContext_Free(&spirv);
		}
	}

	/*Create Tecchniques*/


	/*Cleanup*/
	for (int i = 0; i < glslTemplateCount; i++)
		glslGenContext_Free(&glslTemplates[i]);
	arrfree(glslTemplateNames);
	arrfree(glslTemplates);
	arrfree(glslShaderDescs);
	fxAssemblerRelease(fx);
	free(tmp);
	return result;
}

void shaderGenResult_Free(shaderGenResult_t* res)
{
	arrfree(res->pContents);
}