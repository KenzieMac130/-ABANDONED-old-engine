#include "engine/common/asCommon.h"
#include "engine/renderer/asShaderVariants.h"

#include "GLSLCompiler.h"
#include "FxCreator.h"

/*argv[1]: Input .glsl, argv[2]: Output .asfx, argv[3] Target: "SPIR-V", argv[4] Shader Class*/
int main(int argc, char* argv[])
{
	asDebugLog("Running Shader Compiler...");

	char assetPath[1024];
	memset(assetPath, 0, 1024);
	char outputPath[1024];
	memset(outputPath, 0, 1024);
	char target[32];
	memset(target, 0, 32);
	char shaderType[32];
	memset(shaderType, 0, 32);

	/*Config*/
	if (argc < 2) /*Asset Path*/
	{
		asDebugLog("Input (full) Working Asset File Path:");
		fgets(assetPath, 1024, stdin);
		for (int i = 0; i < 1024; i++) { if (assetPath[i] == '\n') { assetPath[i] = '\0'; } }
	}
	else
	{
		strncat(assetPath, argv[1], 1023);
		asDebugLog("%s", assetPath);
	}
	if (argc < 3) /*Output Path*/
	{
		asDebugLog("Input (full) Output Asset File Path:");
		fgets(outputPath, 1024, stdin);
		for (int i = 0; i < 1024; i++) { if (outputPath[i] == '\n') { outputPath[i] = '\0'; } }
	}
	else
	{
		strncat(outputPath, argv[2], 1023);
		asDebugLog("%s", outputPath);
	}
	if (argc < 4) /*Target*/
	{
		strncat(target, "SPIR-V", 31);
	}
	else
	{
		strncat(target, argv[3], 31);
		asDebugLog("%s", target);
	}

	/*Open File*/
	FILE* fp = fopen(assetPath, "rb");
	if (!fp)
	{
		asDebugLog("[ERROR]> Could not open file: %s", assetPath);
		return 3;
	}
	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* glslContents = asMalloc(fileSize + 1);
	ASASSERT(glslContents);
	memset(glslContents, 0, fileSize + 1);
	fread(glslContents, fileSize, 1, fp);
	fclose(fp);

	/*Search File for Shader Type Macro*/
	int foundPos = re_match("#pragma asShaderType ", glslContents);
	if (foundPos <= 0) { return 4; }
	foundPos += 21;
	if (foundPos > fileSize) { return 4; }
	int endPos = re_match("\\s", &glslContents[foundPos]);
	strncat(shaderType, &glslContents[foundPos], endPos > 31 ? 31 : endPos);

	/*Find Shader Type*/
	asShaderTypeRegistration* shaderTypeInfo = asShaderFindTypeRegistrationByName(shaderType);
	if (!shaderTypeInfo)
	{
		asDebugLog("[ERROR]> Could not find shader fx variant: %s", shaderType);
		return 1;
	}

	/*Create FX file*/
	FxCreator fxCreator;
	if (FxCreator_Create(&fxCreator, outputPath, shaderType) != AS_SUCCESS)
	{
		asDebugLog("[ERROR]> Could not create file: %s", outputPath);
		return 2;
	}

	/*Compile all Permutations*/
	for (int j = 0; j < shaderTypeInfo->codePathCount; j++) /*Todo: Fix*/
	{
		glslToSpirv(&fxCreator, 
			glslContents, fileSize,
			assetPath,
			shaderTypeInfo->codePaths[j].pipelineName,
			&shaderTypeInfo->codePaths[j]);
	}

	FxCreator_Finish(&fxCreator);
	asFree(glslContents);
	//system("pause");
	return 0;
}