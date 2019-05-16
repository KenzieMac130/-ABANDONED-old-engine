#include "SPIRVCompiler.h"

#include "shaderc/shaderc.h"

#include "cute/cute_files.h"
#include "stb/stb_ds.h"

shaderc_shader_kind shadercKindFromNative(asShaderStage stage)
{
	switch (stage)
	{
	case AS_SHADERSTAGE_VERTEX:
		return shaderc_vertex_shader;
	case AS_SHADERSTAGE_TESS_CONTROL:
		return shaderc_tess_control_shader;
	case AS_SHADERSTAGE_TESS_EVALUATION:
		return shaderc_tess_evaluation_shader;
	case AS_SHADERSTAGE_GEOMETRY:
		return shaderc_geometry_shader;
	case AS_SHADERSTAGE_FRAGMENT:
		return shaderc_fragment_shader;
	case AS_SHADERSTAGE_COMPUTE:
		return shaderc_compute_shader;
	default:
		return shaderc_glsl_infer_from_source;
	}
}

static shaderc_include_result* findIncludeCB(void* user_data, const char* requested_source, int type, const char* requesting_source, size_t include_depth)
{
	spirvGenContext_t* ctx = (spirvGenContext_t*)user_data;
	char* fullPath = NULL;
	int includePathLenth = strlen(ctx->includePath);
	int requestedLength = strlen(requested_source);
	arrsetlen(fullPath, (includePathLenth + requestedLength + 1));
	memcpy(fullPath, ctx->includePath, includePathLenth);
	memcpy(fullPath+ includePathLenth, requested_source, requestedLength + 1);

	char* fileBytes = NULL;
	int fileSize = 0;
	{
		FILE *fp;
		fopen_s(&fp, fullPath, "rb");
		if (!fp) {
			asDebugLog("ERROR: UNABLE TO OPEN %s!\n", fullPath);
		}
		else
		{
			fseek(fp, 0, SEEK_END);
			fileSize = ftell(fp) + 1;
			fseek(fp, 0, SEEK_SET);
			fileBytes = (char*)asMalloc(fileSize);
			fread(fileBytes, 1, fileSize, fp);
			fileBytes[fileSize - 1] = '\0';
			fclose(fp);
		}
	}

	shaderc_include_result* result = (shaderc_include_result*)calloc(1, sizeof(shaderc_include_result));
	result->user_data = user_data;
	if (!fileBytes)
	{
		arrfree(fullPath);
		result->source_name = NULL;
		result->source_name_length = 0;
	}
	else 
	{
		result->content = fileBytes;
		result->content_length = fileSize - 1;
		result->source_name = fullPath;
		result->source_name_length = strlen(fullPath);
	}
	return result;
}

static void releaseIncludeCB(void* user_data, shaderc_include_result* include_result)
{
	asFree(include_result->content);
	arrfree(include_result->source_name);
	free(include_result);
}

int genSpirvFromGlsl(spirvGenContext_t* ctx, glslGenContext_t* glsl, asShaderStage stage)
{
	ctx->compiler = shaderc_compiler_initialize();
	if (!ctx->compiler)
		return UINT32_MAX;

	shaderc_compile_options_t compilerOptions = shaderc_compile_options_initialize();
	shaderc_compile_options_set_include_callbacks(compilerOptions, findIncludeCB, releaseIncludeCB, ctx);
	shaderc_compile_options_set_optimization_level(compilerOptions, shaderc_optimization_level_performance);
	shaderc_compile_options_set_source_language(compilerOptions, shaderc_source_language_glsl);

	for (int i = 0; i < ctx->macroCount; i++)
		shaderc_compile_options_add_macro_definition(compilerOptions,
			ctx->pMacros->name, strlen(ctx->pMacros->name),
			ctx->pMacros->value, strlen(ctx->pMacros->value));

	ctx->result = shaderc_compile_into_spv(ctx->compiler, glsl->text, glsl->size,
		shadercKindFromNative(stage), "GENERATED", "main", compilerOptions);
	asDebugLog(shaderc_result_get_error_message(ctx->result));

	int numErrors = (int)shaderc_result_get_num_errors(ctx->result);
	if (!numErrors)
		asDebugLog("GLSL->SPIRV Compile Successful!\n");
	return numErrors;
}

size_t spirvGenContext_GetLength(spirvGenContext_t* ctx)
{
	return shaderc_result_get_length(ctx->result);
}

void* spirvGenContext_GetBytes(spirvGenContext_t* ctx)
{
	return (void*)shaderc_result_get_bytes(ctx->result);
}

void spirvGenContext_Free(spirvGenContext_t* ctx)
{
	shaderc_result_release(ctx->result);
	shaderc_compiler_release(ctx->compiler);
}