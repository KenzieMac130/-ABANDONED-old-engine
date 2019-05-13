#include "GLSLGenerator.h"
#include "stb/stb_ds.h"

void loadGLSLFromFile(glslGenContext_t* ctx, const char* filePath)
{
	FILE* fp = fopen(filePath, "rb");
	if (!fp)
		return;
	fseek(fp, 0, SEEK_END);
	ctx->size = ftell(fp) + 1;
	fseek(fp, 0, SEEK_SET);
	arrsetlen(ctx->byteArray, ctx->size);
	fread(ctx->byteArray, ctx->size, 1, fp);
	ctx->byteArray[ctx->size - 1] = 0;
	fclose(fp);
}

void appendInputLayout(glslGenContext_t* ctx, int set, int binding, const char* name, const char* type)
{
	size_t size;
	size_t insertPos;
	char buff[128];
	memset(buff, 0, 128);
	snprintf(buff, 128, "layout(set = %d, binding = %d) %s %s;\n", set, binding, type, name);
	size = strlen(buff);
	insertPos = arrlen(ctx->byteArray);
	arrsetlen(ctx->byteArray, insertPos + size);
	memcpy(ctx->byteArray + insertPos, buff, size);
}

int genInputs(glslGenContext_t* ctx, fxContext_t* fx, int baseSet)
{
	/*Samplers*/
	appendInputLayout(ctx, baseSet + 0,
		0, "DEFAULTSAMPLER", "uniform sampler");
	appendInputLayout(ctx, baseSet + 0,
		1, "NEARESTSAMPLER", "uniform sampler");
	appendInputLayout(ctx, baseSet + 0,
		2, "CLAMPEDSAMPLER", "uniform sampler");
	/*Application Variables*/
	appendInputLayout(ctx, baseSet + 1,
		0, "app", 
		"uniform asFxAppVar_t {\n"
		"\tmat4 view;\n"
		"\tmat4 proj;\n"
		"\tvec4 custom;\n"
		"\tfloat time;\n"
		"\tint DebugMode;\n}");
	/*Material Values*/
	{
		char* bufferGen = NULL;
		uint32_t bufferPos = 29;
		uint32_t lineSize;
		const char* type;
		const char* varName;
		arrsetcap(bufferGen, 512);
		const char *header = "uniform genProps_t {\n";
		arrsetlen(bufferGen, 29);
		memcpy(bufferGen, header, 29);
		for (uint32_t i = 0; i < fx->desc.propCount; i++)
		{
			if (fx->desc.pProps[i].type == AS_SHADERFXPROP_SCALAR)
				type = "float";
			else if (fx->desc.pProps[i].type == AS_SHADERFXPROP_VECTOR4)
				type = "vec4";
			else
				continue;
			varName = strpool_cstr(&fx->stringPool, fx->pFxPropNames[i]);
			lineSize = snprintf(NULL, 0, "\t%s %s;\n", type, varName);
			arrsetlen(bufferGen, bufferPos + lineSize);
			snprintf(bufferGen + bufferPos, lineSize, "\t%s %s;", type, varName);
			bufferPos += lineSize;
			bufferGen[bufferPos-1] = '\n';
		}
		arrsetlen(bufferGen, bufferPos + 2);
		bufferGen[bufferPos] = '}';
		bufferGen[bufferPos+1] = '\0';
		appendInputLayout(ctx, baseSet + 2, 0, "mat", bufferGen);
	}
	/*Material Textures*/
	{
		for (uint32_t i = 0; i < fx->desc.propCount; i++)
		{
			const char* type;
			const char* varName;
			switch (fx->desc.pProps[i].type)
			{
			default:
				continue;
			case AS_SHADERFXPROP_TEX2D:
				type = "texture2D";
				break;
			case AS_SHADERFXPROP_TEX2DARRAY:
				type = "texture2DArray";
				break;
			case AS_SHADERFXPROP_TEXCUBE:
				type = "textureCube";
				break;
			case AS_SHADERFXPROP_TEXCUBEARRAY:
				type = "textureCubeArray";
				break;
			case AS_SHADERFXPROP_TEX3D:
				type = "texture3D";
				break;
			}
			varName = strpool_cstr(&fx->stringPool, fx->pFxPropNames[i]);
			appendInputLayout(ctx, baseSet + 2, fx->desc.pProps[i].offset, varName, type);
		}
	}
	return 3;
}

void appendDescSet(glslGenContext_t* ctx, fxContext_t* fx, int set)
{
	size_t size;
	size_t insertPos;
	char buff[16];
	memset(buff, 0, 16);
	snprintf(buff, 16, "set = %d\0", set);
	size = strlen(buff);
	insertPos = arrlen(ctx->byteArray);
	arrsetlen(ctx->byteArray, insertPos + size);
	memcpy(ctx->byteArray + insertPos, buff, size);
}
void appendSnippet(glslGenContext_t* ctx, fxContext_t* fx, char* snippetName)
{
	asHash64_t snippetNameHash = asHashBytes64_xxHash(snippetName, strlen(snippetName));
	const char* snippetStr;
	size_t snippetSize;
	size_t insertPos;
	for (size_t i = 0; i < arrlen(fx->pGenNameHashes); i++)
	{
		if (fx->pGenNameHashes[i] == snippetNameHash)
		{
			snippetStr = strpool_cstr(&fx->stringPool, fx->pGenValues[i]);
			snippetSize = strlen(snippetStr);
			insertPos = arrlen(ctx->byteArray);
			arrsetlen(ctx->byteArray, insertPos + snippetSize);
			memcpy(ctx->byteArray + insertPos, snippetStr, snippetSize);
			return;
		}
	}
}

void genFromComment(glslGenContext_t* ctx, fxContext_t* fx, char* parsePos, int* nextSet)
{
	while (parsePos[0] == ' ')
		parsePos++;
	/*Inside Fx Namespace*/
	if (!strncmp(parsePos, "Fx::", 4))
	{
		parsePos += 4;
		if (!strncmp(parsePos, "Snippet::", 9))
		{
			parsePos += 9;
			/*Null terminate snippet name at whitespace*/
			for(int i = 0; i < strlen(parsePos); i++) {
				if (parsePos[i] == ' ' || parsePos[i] == '\n' || parsePos[i] == '\r') {
					parsePos[i] = '\0';
					break;
				}
			}
			/*Append Snippet*/
			appendSnippet(ctx, fx, parsePos);
		}
		else if (!strncmp(parsePos, "DescSet", 7))
		{
			parsePos += 7;
			if (!strncmp(parsePos, "::Next", 6))
			{
				++*nextSet;
			}
			appendDescSet(ctx, fx, *nextSet);
		}
		else if (!strncmp(parsePos, "Inputs", 6))
		{
			/*Generate Inputs*/
			*nextSet += genInputs(ctx, fx, *nextSet);
		}
	}
}

void appendHeader(glslGenContext_t* ctx, fxContext_t* fx)
{
	const char* topOfFile =
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n";
	const size_t size = strlen(topOfFile);
	size_t insertPos;
	insertPos = arrlen(ctx->byteArray);
	arrsetlen(ctx->byteArray, insertPos + size);
	memcpy(ctx->byteArray + insertPos, topOfFile, size);
}

void generateGLSLFxFromTemplate(glslGenContext_t* ctx, fxContext_t* fx, glslGenContext_t* templateGlsl)
{
	/*Parse until you hit a comment*/
	bool inCommentSection = false;
	bool singleLineComment = false;
	char* commentText = NULL;
	int nextDescSet = 0;
	arrsetcap(commentText, arrcap(commentText) + 512);
	arrsetcap(ctx->byteArray, arrcap(ctx->byteArray) + templateGlsl->size);
	appendHeader(ctx, fx);
	for (size_t i = 0; i < templateGlsl->size - 1; i++)
	{
		/*Entered Comment Section*/
		if (!inCommentSection)
		{
			if (templateGlsl->byteArray[i] == '/' && templateGlsl->byteArray[i + 1] == '/')
			{
				inCommentSection = true;
				singleLineComment = true;
				i++;
				continue;
			}
			else if (templateGlsl->byteArray[i] == '/' && templateGlsl->byteArray[i + 1] == '*')
			{
				inCommentSection = true;
				singleLineComment = false;
				i++;
				continue;
			}
		}
		else
		{
			/*Exited Comment Section*/
			if (singleLineComment)
			{
				if ((templateGlsl->byteArray[i] == '\n') || (templateGlsl->byteArray[i] == '\r'))
				{
					arrput(commentText, '\0');
					genFromComment(ctx, fx, commentText, &nextDescSet);
					inCommentSection = false;
					i++;
					arrsetlen(commentText, 0);
					continue;
				}
			}
			else
			{
				if (templateGlsl->byteArray[i] == '*' && templateGlsl->byteArray[i + 1] == '/')
				{
					arrput(commentText, '\0');
					genFromComment(ctx, fx, commentText, &nextDescSet);
					inCommentSection = false;
					i++;
					arrsetlen(commentText, 0);
					continue;
				}
			}
		}
		/*Append to Comment*/
		if (inCommentSection)
		{
			arrput(commentText, templateGlsl->byteArray[i]);
		}
		else
		{
			arrput(ctx->byteArray, templateGlsl->byteArray[i]);
		}
	}
	arrput(ctx->byteArray, '\0');
	ctx->size = strlen(ctx->byteArray);
	asDebugLog(ctx->byteArray);
}
void glslGenContext_Free(glslGenContext_t* ctx)
{
	arrfree(ctx->byteArray);
}