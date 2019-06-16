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
		return shaderc_vertex_shader;
	}
}

static shaderc_include_result* findIncludeCB(void* user_data, const char* requested_source, int type, const char* requesting_source, size_t include_depth)
{
	cSpirvGenContext* ctx = (cSpirvGenContext*)user_data;
	char* fullPath = NULL;
	int includePathLenth = (int)strlen(ctx->includePath);
	int requestedLength = (int)strlen(requested_source);
	arrsetlen(fullPath, (includePathLenth + requestedLength + 1));
	memcpy(fullPath, ctx->includePath, includePathLenth);
	memcpy(fullPath+ includePathLenth, requested_source, requestedLength + 1);

	char* fileBytes = NULL;
	int fileSize = 0;
	{
		FILE *fp;
		fopen_s(&fp, fullPath, "rb");
		if (!fp) {
			if(ctx->pDebugStream)
				*ctx->pDebugStream << "ERROR: UNABLE TO OPEN" << fullPath << "!\n";
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
	asFree((void*)include_result->content);
	arrfree(include_result->source_name);
	free(include_result);
}

int cSpirvGenContext::genSpirvFromGlsl(cGlslGenContext* glsl)
{
	cSpirvGenContext* ctx = this;
	ctx->compiler = shaderc_compiler_initialize();
	if (!ctx->compiler)
		return UINT32_MAX;

	shaderc_compile_options_t compilerOptions = shaderc_compile_options_initialize();
	shaderc_compile_options_set_include_callbacks(compilerOptions, findIncludeCB, releaseIncludeCB, ctx);
	shaderc_compile_options_set_warnings_as_errors(compilerOptions);
	shaderc_compile_options_set_optimization_level(compilerOptions, shaderc_optimization_level_performance);
	shaderc_compile_options_set_source_language(compilerOptions, shaderc_source_language_glsl);

	for (int i = 0; i < ctx->macros.size(); i++)
		shaderc_compile_options_add_macro_definition(compilerOptions,
			ctx->macros[i].name, strlen(ctx->macros[i].name),
			ctx->macros[i].value, strlen(ctx->macros[i].value));

	ctx->result = shaderc_compile_into_spv(ctx->compiler, glsl->text.c_str(), glsl->text.size(),
		shadercKindFromNative(glsl->stage), "GENERATED", "main", compilerOptions);

	size_t numErrors = shaderc_result_get_num_errors(ctx->result);
	if(ctx->pDebugStream)
		if (numErrors)
		{
			*ctx->pDebugStream <<
				shaderc_result_get_error_message(ctx->result);
				/*"-------------------------------DUMP--------------------------------\n" <<
				glsl->text <<
				"\n-------------------------------------------------------------------\n";*/
		}
		else
			*ctx->pDebugStream << "GLSL->SPIRV Compile Successful!\n";
	return (int)numErrors;
}

size_t cSpirvGenContext::GetLength()
{
	return shaderc_result_get_length(this->result);
}

void* cSpirvGenContext::GetBytes()
{
	return (void*)shaderc_result_get_bytes(this->result);
}

cSpirvGenContext::cSpirvGenContext()
{
	this->pDebugStream = NULL;
}

cSpirvGenContext::cSpirvGenContext(std::ostringstream *debugStream, const char* includeDir)
{
	this->pDebugStream = debugStream;
	this->compiler = nullptr;
	this->result = nullptr;
	this->includePath = includeDir;
}

cSpirvGenContext::~cSpirvGenContext()
{
	if(this->result)
		shaderc_result_release(this->result);
	if (this->compiler)
		shaderc_compiler_release(this->compiler);
}