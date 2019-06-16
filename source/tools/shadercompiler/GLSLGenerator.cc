#include "GLSLGenerator.h"
#include "stb/stb_ds.h"

void cGlslGenContext::loadGLSLFromFile(const char* filePath)
{
	cGlslGenContext* ctx = this;
	FILE* fp = fopen(filePath, "rb");
	if (!fp)
		return;
	fseek(fp, 0, SEEK_END);
	ctx->text.resize(ftell(fp));
	fseek(fp, 0, SEEK_SET);
	fread((void*)ctx->text.data(), ctx->text.size(), 1, fp);
	fclose(fp);
}

void appendInputLayout(cGlslGenContext* ctx, int set, int binding, const char* name, const char* type)
{
	char buff[2048];
	snprintf(buff, 1024, "layout(set = %d, binding = %d) %s %s;\n", set, binding, type, name);
	ctx->text.append(buff);
}

int genInputs(cGlslGenContext* ctx, cFxContext* fx, int baseSet)
{
	/*Samplers*/
	appendInputLayout(ctx, baseSet + 0,
		0, "SAMPLER_DEFAULT", "uniform sampler");
	appendInputLayout(ctx, baseSet + 0,
		1, "SAMPLER_NEAREST", "uniform sampler");
	appendInputLayout(ctx, baseSet + 0,
		2, "SAMPLER_CLAMPED", "uniform sampler");
	appendInputLayout(ctx, baseSet + 0,
		3, "SAMPLER_CLAMPEDNEAREST", "uniform sampler");
	appendInputLayout(ctx, baseSet + 0,
		4, "SAMPLER_SHADOW", "uniform sampler");
	/*Application Variables*/
	const char* appVariableText =
		"uniform asFxAppVar_t {\n"
		"\tmat4 View;\n"
		"\tmat4 Proj;\n"
		"\tvec4 Custom;\n"
		"\tfloat Time;\n"
		"\tint DebugMode;\n}";
	appendInputLayout(ctx, baseSet + 1, 0, "App", appVariableText);
	/*Global Texture Array*/
	appendInputLayout(ctx, baseSet + 2,
		0, "TEXTURES_2D", "uniform texture2D[512]");
	appendInputLayout(ctx, baseSet + 2,
		1, "TEXTURES_3D", "uniform texture3D[512]");
	appendInputLayout(ctx, baseSet + 2,
		2, "TEXTURES_CUBE", "uniform textureCube[512]");
	/*Material Values*/
	{
		std::string buffGen;
		std::string typeBlock;
		typeBlock.reserve(512);
		buffGen.reserve(512);
		uint32_t lineSize;
		const char* type;
		const char* varName;
		/*Props*/
		for (uint32_t i = 0; i < fx->desc.propCount; i++)
		{
			if (fx->desc.pPropLookup[i].type == AS_SHADERFXPROP_VECTOR4)
			{
				typeBlock.append("\tvec4 ");
			}
			else if (fx->desc.pPropLookup[i].type == AS_SHADERFXPROP_SCALAR)
			{
				typeBlock.append("\tfloat ");
			}
			else
			{
				typeBlock.append("\tint ");
			}
			varName = fx->FxPropNames[i].c_str();
			typeBlock.append(varName);
			typeBlock.append(";\n");
		}
		/*Padding (useful for addressing bindless materials)*/
		{
			typeBlock.append("\tint _padding[");
			typeBlock.append(std::to_string(fx->defaultBufferPadding / 4));
			typeBlock.append("];\n");
		}

		buffGen = "uniform genMat_t {\n";
		buffGen.append(typeBlock);
		buffGen.append("}");
		appendInputLayout(ctx, baseSet + 2,
			3, "Mat", buffGen.c_str());
	}
	return 4;
}

void appendDescSet(cGlslGenContext* ctx, cFxContext* fx, int set)
{
	char buff[16];
	memset(buff, 0, 16);
	snprintf(buff, 16, "set = %d\0", set);
	ctx->text.append(buff);
}
void appendSnippet(cGlslGenContext* ctx, cFxContext* fx, char* snippetName)
{
	asHash64_t snippetNameHash = asHashBytes64_xxHash(snippetName, strlen(snippetName));
	for (int i = 0; i < fx->GenNameHashes.size(); i++)
	{
		if (fx->GenNameHashes[i] == snippetNameHash)
		{
			ctx->text.append(fx->GenValues[i].c_str());
			return;
		}
	}
}

void genFromComment(cGlslGenContext* ctx, cFxContext* fx, char* parsePos, int* nextSet)
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
			else
			{
				appendDescSet(ctx, fx, *nextSet);
			}
		}
		else if (!strncmp(parsePos, "Inputs", 6))
		{
			/*Generate Inputs*/
			*nextSet += genInputs(ctx, fx, *nextSet);
		}
	}
}

void appendHeader(cGlslGenContext* ctx, cFxContext* fx)
{
	const char* topOfFile =
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n";
	ctx->text.append(topOfFile);
}

void cGlslGenContext::generateGLSLFxFromTemplate(cFxContext* fx, cGlslGenContext* templateGlsl)
{
	/*Parse until you hit a comment*/
	cGlslGenContext* ctx = this;
	this->stage = templateGlsl->stage;
	bool inCommentSection = false;
	bool singleLineComment = false;
	char* commentText = NULL;
	int nextDescSet = 0;
	arrsetcap(commentText, 512);
	ctx->text.reserve(templateGlsl->text.size()+2048);
	appendHeader(ctx, fx);
	for (size_t i = 0; i < templateGlsl->text.size(); i++)
	{
		/*Entered Comment Section*/
		if (!inCommentSection)
		{
			if (templateGlsl->text[i] == '/' && templateGlsl->text[i + 1] == '/')
			{
				inCommentSection = true;
				singleLineComment = true;
				i++;
				continue;
			}
			else if (templateGlsl->text[i] == '/' && templateGlsl->text[i + 1] == '*')
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
				if ((templateGlsl->text[i] == '\n') || (templateGlsl->text[i] == '\r'))
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
				if (templateGlsl->text[i] == '*' && templateGlsl->text[i + 1] == '/')
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
			arrput(commentText, templateGlsl->text[i]);
		}
		else
		{
			ctx->text.push_back(templateGlsl->text[i]);
		}
	}
	if (this->pDebugStream)
	{
		*this->pDebugStream << "\nGENERATED GLSL:\n"
			<< "---------------------------------------------------------------\n"
			<< std::string(ctx->text.c_str())
			<< "\n---------------------------------------------------------------\n";
	}
	arrfree(commentText);
}

cGlslGenContext::cGlslGenContext(std::ostringstream *debugStream)
{
	this->pDebugStream = debugStream;
}