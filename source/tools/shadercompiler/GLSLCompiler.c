#include "GLSLCompiler.h"

#include "engine/common/asCommon.h"
#include "shaderc/shaderc.h"

struct IncludeFileData {
	FxCreator* pFxCreator;
	char includeFilePath[1024];
};

#define MAX_DEPTH 64

struct shaderc_include_result* includeFile(void* userData, const char* requestedFile, int type, const char* fileMakingRequest, size_t depth)
{
	/*Break on Depth*/
	if (depth > MAX_DEPTH) 
	{
		asDebugLog("[ERROR]> Max Include depth reached! (cyclic?)");
		return NULL;
	};

	struct IncludeFileData* pIncludeData = userData;
	ASASSERT(pIncludeData);

	/*Create Relative Path*/
	char directory[1024];
	memset(directory, 0, 1024);
	strncat(directory, fileMakingRequest, 1023);
	char* foundPos = strrchr(directory, '/');
	if (!foundPos) { foundPos = strrchr(directory, '\\'); }
	ASASSERT(foundPos);
	*foundPos = '\0';

	/*Create Path*/
	memset(pIncludeData->includeFilePath, 0, 1024);
	snprintf(pIncludeData->includeFilePath, 1023, "%s/%s", directory, requestedFile);

	/*Open File*/
	FILE* fp = fopen(pIncludeData->includeFilePath, "rb");
	if (!fp)
	{
		asDebugLog("[ERROR]> Could not open file: %s", pIncludeData->includeFilePath);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* glslContents = asMalloc(fileSize + 1);
	ASASSERT(glslContents);
	memset(glslContents, 0, fileSize + 1);
	fread(glslContents, fileSize, 1, fp);
	fclose(fp);

	/*Process Reflection Macros for Includes*/
	processMacros(&pIncludeData->pFxCreator, glslContents, fileSize, requestedFile);

	/*Return Result*/
	struct shaderc_include_result* result = asMalloc(sizeof(struct shaderc_include_result));
	ASASSERT(result);
	memset(result, 0, sizeof(struct shaderc_include_result));
	result->user_data = userData;
	result->content = glslContents;
	result->content_length = fileSize;
	result->source_name = pIncludeData->includeFilePath;
	result->source_name_length = strlen(pIncludeData->includeFilePath);
	return result;
}

void releaseIncludeResult(void* userData, shaderc_include_result* result)
{
	asFree(result->content);
	asFree(result);
}

shaderc_shader_kind getShaderKind(asShaderStage stage)
{
	switch (stage)
	{
		case AS_SHADERSTAGE_VERTEX: return shaderc_vertex_shader;
		case AS_SHADERSTAGE_FRAGMENT: return shaderc_fragment_shader;
		case AS_SHADERSTAGE_TESS_CONTROL: return shaderc_tess_control_shader;
		case AS_SHADERSTAGE_TESS_EVALUATION: return shaderc_tess_evaluation_shader;
		case AS_SHADERSTAGE_GEOMETRY: return shaderc_geometry_shader;
		case AS_SHADERSTAGE_COMPUTE: return shaderc_compute_shader;
		default: return shaderc_glsl_infer_from_source;
	}
}

asResults glslToSpirv(FxCreator* pFxCreator, const char* contents, size_t contentsSize, const char* fileName, const char* pipelineName, const asShaderTypeCodePath* codePath)
{
	/*Setup Compiler*/
	struct shaderc_compiler* glslCompiler = shaderc_compiler_initialize();
	struct shaderc_compile_options* compileOptions = shaderc_compile_options_initialize();
	
	shaderc_compile_options_set_target_env(compileOptions, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
	shaderc_compile_options_set_source_language(compileOptions, shaderc_source_language_glsl);
	shaderc_compile_options_set_optimization_level(compileOptions, shaderc_optimization_level_performance);
	shaderc_compile_options_set_warnings_as_errors(compileOptions);

	/*File Includer*/
	struct IncludeFileData includeData = { pFxCreator };
	shaderc_compile_options_set_include_callbacks(compileOptions, includeFile, releaseIncludeResult, &includeData);

	/*Add Stage Macro*/
	const char* stageName = "AS_SHADER_STAGE_UNKNOWN";
	switch (codePath->stage){
	case AS_SHADERSTAGE_VERTEX: stageName = "AS_STAGE_VERTEX"; break;
	case AS_SHADERSTAGE_FRAGMENT: stageName = "AS_STAGE_FRAGMENT"; break;
	case AS_SHADERSTAGE_TESS_CONTROL: stageName = "AS_STAGE_TESS_CONTROL"; break;
	case AS_SHADERSTAGE_TESS_EVALUATION: stageName = "AS_STAGE_TESS_EVALUATION"; break;
	case AS_SHADERSTAGE_GEOMETRY: stageName = "AS_STAGE_GEOMETRY"; break;
	case AS_SHADERSTAGE_COMPUTE: stageName = "AS_STAGE_COMPUTE"; break;}
	shaderc_compile_options_add_macro_definition(compileOptions, stageName, strlen(stageName), "", 0);

	/*Add Quality Macro*/
	char qualityInt[8];
	memset(qualityInt, 0, 8);
	snprintf(qualityInt, 7, "%i", codePath->minQuality);
	shaderc_compile_options_add_macro_definition(compileOptions, "AS_MIN_QUALITY", 14, qualityInt, strlen(qualityInt));

	/*Add Extra Macros*/
	for (int i = 0; i < codePath->macroCount; i++)
	{
		shaderc_compile_options_add_macro_definition(compileOptions,
			codePath->macros[i].name, strlen(codePath->macros[i].name),
			codePath->macros[i].value, strlen(codePath->macros[i].value));
	}

	/*Process Reflection Macros for Top Level*/
	processMacros(&pFxCreator, contents, contentsSize, fileName);

	/*Compile*/
	shaderc_compilation_result_t result = shaderc_compile_into_spv(
		glslCompiler,
		contents,
		contentsSize,
		getShaderKind(codePath->stage),
		fileName, codePath->entry,
		compileOptions);

	/*Error Handling*/
	shaderc_compilation_status status = shaderc_result_get_compilation_status(result);
	if (status != shaderc_compilation_status_success)
	{
		const char* error = shaderc_result_get_error_message(result);
		asDebugLog("[ERROR]> Pipeline: \"%s\" Error: \"%s\"", pipelineName, error);
		//if (getchar()) {};
		_fcloseall();
		exit(status);
	}

	/*Add Block*/
	FxCreator_AddCodeBlock(pFxCreator,
		pipelineName,
		shaderc_result_get_bytes(result),
		shaderc_result_get_length(result),
		codePath->stage,
		codePath->minQuality);

	shaderc_compile_options_release(compileOptions);
	shaderc_compiler_release(glslCompiler);

	return AS_SUCCESS;
}