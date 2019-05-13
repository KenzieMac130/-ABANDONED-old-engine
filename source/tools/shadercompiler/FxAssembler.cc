#include "FxAssembler.h"
#include "stb/stb_ds.h"

fxContext_t* fxAssemblerInit()
{
	fxContext_t *ctx = (fxContext_t*)asMalloc(sizeof(fxContext_t));
	memset(ctx, 0, sizeof(fxContext_t));
	ctx->desc = asShaderFxDesc_Init();
	strpool_config_t conf = strpool_default_config;
	strpool_init(&ctx->stringPool, &conf);
	return ctx;
}

void fxAssemblerRelease(fxContext_t *ctx)
{
	arrfree(ctx->desc.pProps);
	arrfree(ctx->pDependencies);
	arrfree(ctx->pFxPropNames);
	arrfree(ctx->pGenNameHashes);
	arrfree(ctx->pGenValues);
	strpool_term(&ctx->stringPool);
	asFree(ctx);
}

/*Material Props*/

char* getNextMaterialProperty(char* input, size_t inputSize,
	asShaderFxPropType *outPropType,
	char* outName, size_t outNameMax,
	float *outDefaults)
{
	char* startPos;
	char* endPos;
	char* typeStartPos;
	char* typeEndPos;
	char* nameStartPos;
	char* nameEndPos;
	char* defaultStartPos;
	char* defaultEndPos;
	char tmpBuff[512];
	startPos = strchr(input, '{');
	if ((size_t)(startPos - input) > inputSize)
		return NULL;
	/*Find start*/
	if (startPos == NULL) {
		return NULL;
	}
	startPos++;
	/*Find end*/
	endPos = strchr(startPos, '}');
	if (endPos == NULL) {
		return NULL;
	}

	/*Find type start and end position*/
	typeStartPos = startPos;
	while (*typeStartPos == ' ')
		typeStartPos++;
	typeEndPos = typeStartPos;
	while (*typeEndPos != ' ' && *typeEndPos != ',')
		typeEndPos++;
	strncpy_s(tmpBuff, 512, typeStartPos, (typeEndPos - typeStartPos));
	tmpBuff[(typeEndPos - typeStartPos)] = '\0';
	if (strcmp(tmpBuff, "Scalar") == 0){
		/*Type is scalar*/
		*outPropType = AS_SHADERFXPROP_SCALAR;
	}
	else if (strcmp(tmpBuff, "Vec4") == 0){
		/*Type is vector4*/
		*outPropType = AS_SHADERFXPROP_VECTOR4;
	}
	else if (strcmp(tmpBuff, "Tex2D") == 0){
		/*Type is Texture 2D*/
		*outPropType = AS_SHADERFXPROP_TEX2D;
	}
	else if (strcmp(tmpBuff, "Tex2DArray") == 0){
		/*Type is Texture 2D Array*/
		*outPropType = AS_SHADERFXPROP_TEX2DARRAY;
	}
	else if (strcmp(tmpBuff, "TexCube") == 0){
		/*Type is Texture Cube*/
		*outPropType = AS_SHADERFXPROP_TEXCUBE;
	}
	else if (strcmp(tmpBuff, "TexCubeArray") == 0){
		/*Type is Texture Cube Array*/
		*outPropType = AS_SHADERFXPROP_TEXCUBEARRAY;
	}
	else if (strcmp(tmpBuff, "Tex3D") == 0) {
		/*Type is Texture 3D*/
		*outPropType = AS_SHADERFXPROP_TEX3D;
	}
	else {
		asFatalError("Type: \"%s\" Not Found");
	}

	/*Get name*/
	nameStartPos = typeEndPos;
	while (*nameStartPos != '\"')
		nameStartPos++;
	nameStartPos++;
	nameEndPos = nameStartPos;
	while (*nameEndPos != '\"')
		nameEndPos++;
	strncpy_s(outName, outNameMax, nameStartPos, (nameEndPos - nameStartPos));
	outName[(nameEndPos - nameStartPos)] = '\0';

	/*Retrieve defaults*/
	defaultStartPos = nameEndPos+1;
	while (*defaultStartPos == ' ' || *defaultStartPos == ',')
		defaultStartPos++;
	switch (*outPropType)
	{
	/*Properties*/
	case AS_SHADERFXPROP_SCALAR:
			outDefaults[0] = strtof(defaultStartPos, &defaultEndPos);
			break;

	case AS_SHADERFXPROP_VECTOR4:
		if (*defaultStartPos != '(')
			break;
		defaultStartPos++;
		for (int i = 0; i < 4; i++)
		{
			if (*defaultStartPos == ')')
				break;
			outDefaults[i] = strtof(defaultStartPos, &defaultEndPos);
			defaultStartPos = defaultEndPos;
			while (*defaultStartPos == ' ' || *defaultStartPos == ',')
				defaultStartPos++;
		}
		break;
	/*Textures*/
	default: 
		defaultEndPos = defaultStartPos;
		while (*defaultEndPos != ' ' && *defaultEndPos != '}')
			defaultEndPos++;
		strncpy_s(tmpBuff, 512, defaultStartPos, (defaultEndPos - defaultStartPos));
		tmpBuff[(defaultEndPos - defaultStartPos)] = '\0';
		if (strcmp(tmpBuff, "BLACK") == 0) {
			outDefaults[0] = 0.0f; 
			outDefaults[1] = 0.0f;
			outDefaults[2] = 0.0f;
			outDefaults[3] = 1.0f;
		}
		else if (strcmp(tmpBuff, "WHITE") == 0) {
			outDefaults[0] = 1.0f;
			outDefaults[1] = 1.0f;
			outDefaults[2] = 1.0f;
			outDefaults[3] = 1.0f;
		}
		else if (strcmp(tmpBuff, "GREY") == 0) {
			outDefaults[0] = 0.5f;
			outDefaults[1] = 0.5f;
			outDefaults[2] = 0.5f;
			outDefaults[3] = 1.0f;
		}
		else if (strcmp(tmpBuff, "NORMAL") == 0) {
			outDefaults[0] = 0.5f;
			outDefaults[1] = 0.5f;
			outDefaults[2] = 1.0f;
			outDefaults[3] = 0.5f;
		}
		else if (strcmp(tmpBuff, "INVISIBLE") == 0) {
			outDefaults[0] = 1.0f;
			outDefaults[1] = 1.0f;
			outDefaults[2] = 1.0f;
			outDefaults[3] = 0.0f;
		}
		break;
	}

	return endPos;
}

void fxAssemblerSetupMaterialProps(fxContext_t *ctx, char* input, size_t inputSize)
{
	/*Parse each material property*/
	asShaderFxPropType propType = (asShaderFxPropType)-1;
	char nameBuff[512];
	float defaults[4];
	asShaderFxProp_t member;
	uint32_t textureSlot = 1; /*Start at 1 to make room for the material*/
	uint32_t bufferOffset = 0;
	char* parsePos = input;
	while (parsePos < input + inputSize)
	{
		/*Next property*/
		memset(defaults, 0, sizeof(float) * 4);
		parsePos = getNextMaterialProperty(parsePos, (input + inputSize) - parsePos, &propType, nameBuff, 512, defaults);
		if (!parsePos)
			break;
		/*Material Members*/
		member = asShaderFxProp_t();
		member.type = propType;
		if (propType == AS_SHADERFXPROP_SCALAR)
		{
			member.offset = bufferOffset;
			bufferOffset += sizeof(float);
			ctx->desc.propBufferSize = bufferOffset;
		}
		else if (propType == AS_SHADERFXPROP_VECTOR4)
		{
			member.offset = bufferOffset;
			bufferOffset += bufferOffset % (sizeof(float) * 4); /*Enforce vulkan alignment for vectors*/
			bufferOffset += sizeof(float) * 4;
			ctx->desc.propBufferSize = bufferOffset;
		}
		else
		{
			member.offset = textureSlot;
			textureSlot++;
		}
		member.nameHash = asHashBytes64_xxHash(nameBuff, strlen(nameBuff));
		member.values[0] = defaults[0];
		member.values[1] = defaults[1];
		member.values[2] = defaults[2];
		member.values[3] = defaults[3];
		arrput(ctx->desc.pProps, member);
		arrput(ctx->pFxPropNames, strpool_inject(&ctx->stringPool, nameBuff, (int)strlen(nameBuff)));
		ctx->desc.propCount++;
	}
}

void fxAssemblerSetupFixedFunctionProps(fxContext_t *ctx, char* input, size_t inputSize)
{
	asCfgFile_t* cfg = (asCfgFile_t*)asCfgFromMem((unsigned char*)input, inputSize);
	const char* tmpStr;
	ctx->desc.fixedFunctions.fillWidth = (float)asCfgGetNumber(cfg, "fillWidth", 1.0f);
	ctx->desc.fixedFunctions.tessCtrlPoints = (uint32_t)asCfgGetNumber(cfg, "tessCtrlPoints", 0.0f);
	/*Fill mode*/
	tmpStr = asCfgGetString(cfg, "fillMode", "FULL");
	if (strcmp(tmpStr, "FULL") == 0)
		ctx->desc.fixedFunctions.fillMode = AS_FILL_FULL;
	else if (strcmp(tmpStr, "WIRE") == 0)
		ctx->desc.fixedFunctions.fillMode = AS_FILL_WIRE;
	else if (strcmp(tmpStr, "POINTS") == 0)
		ctx->desc.fixedFunctions.fillMode = AS_FILL_POINTS;
	/*Cull mode*/
	tmpStr = asCfgGetString(cfg, "cullMode", "BACK");
	if (strcmp(tmpStr, "BACK") == 0)
		ctx->desc.fixedFunctions.cullMode = AS_CULL_BACK;
	else if (strcmp(tmpStr, "FRONT") == 0)
		ctx->desc.fixedFunctions.cullMode = AS_CULL_FRONT;
	else if (strcmp(tmpStr, "NONE") == 0)
		ctx->desc.fixedFunctions.cullMode = AS_CULL_NONE;
	/*Blend mode*/
	tmpStr = asCfgGetString(cfg, "blendMode", "NONE");
	if (strcmp(tmpStr, "NONE") == 0)
		ctx->desc.fixedFunctions.blendMode = AS_BLEND_NONE;
	else if (strcmp(tmpStr, "MIX") == 0)
		ctx->desc.fixedFunctions.blendMode = AS_BLEND_MIX;
	else if (strcmp(tmpStr, "ADD") == 0)
		ctx->desc.fixedFunctions.blendMode = AS_BLEND_ADD;
	asCfgFree(cfg);
}

void fxAssemblerAddDependency(fxContext_t *ctx, char* fileName, size_t nameSize)
{
	STRPOOL_U64 strId = strpool_inject(&ctx->stringPool, fileName, (int)nameSize);
	arrput(ctx->pDependencies, strId);
}

void fxAssemblerAddGeneratorProp(fxContext_t *ctx, char* name, size_t nameSize, char* snippet, size_t snippetSize)
{
	STRPOOL_U64 nameHash = asHashBytes64_xxHash(name, nameSize);
	STRPOOL_U64 valueId = strpool_inject(&ctx->stringPool, snippet, (int)snippetSize);
	arrput(ctx->pGenNameHashes, nameHash);
	arrput(ctx->pGenValues, valueId);
}