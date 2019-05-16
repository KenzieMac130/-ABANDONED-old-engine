#include "FxAssembler.h"
#include "stb/stb_ds.h"

fxContext_t* fxAssemblerInit()
{
	fxContext_t *ctx = (fxContext_t*)asMalloc(sizeof(fxContext_t));
	memset(ctx, 0, sizeof(fxContext_t));
	ctx->desc = asShaderFxDesc_Init();
	strpool_config_t conf = strpool_default_config;
	strpool_init(&ctx->stringPool, &conf);
	arrsetcap(ctx->desc.pShaderCode, 1024);
	return ctx;
}

void fxAssemblerRelease(fxContext_t *ctx)
{
	arrfree(ctx->desc.pProp_Lookup);
	arrfree(ctx->desc.pProp_Offset);
	for (size_t i = 0; i < ctx->desc.techniqueCount; i++)
		arrfree(ctx->desc.pTechniques[i].pPrograms);
	arrfree(ctx->desc.pTechniqueNameHashes);
	arrfree(ctx->desc.pTechniques);
	arrfree(ctx->desc.pShaderCode);
	arrfree(ctx->desc.pPropBufferDefault);
	arrfree(ctx->desc.pPropTextureDefaults);
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
		asDebugLog("Type: \"%s\" Not Found", tmpBuff);
		return NULL;
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
			outDefaults[0] = (float)AS_SHADERFXTEXDEFAULT_BLACK;
		}
		else if (strcmp(tmpBuff, "WHITE") == 0) {
			outDefaults[0] = (float)AS_SHADERFXTEXDEFAULT_WHITE;
		}
		else if (strcmp(tmpBuff, "GREY") == 0) {
			outDefaults[0] = (float)AS_SHADERFXTEXDEFAULT_GREY;
		}
		else if (strcmp(tmpBuff, "NORMAL") == 0) {
			outDefaults[0] = (float)AS_SHADERFXTEXDEFAULT_NORMAL;
		}
		else if (strcmp(tmpBuff, "ALPHA") == 0) {
			outDefaults[0] = (float)AS_SHADERFXTEXDEFAULT_ALPHA;
		}
		else {
			outDefaults[0] = (float)AS_SHADERFXTEXDEFAULT_BLACK;
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
	uint32_t textureSlot = 1; /*Start at 1 to make room for the material*/
	uint32_t bufferOffset = 0;
	char* parsePos = input;
	arrsetcap(ctx->pDefaultBuffer, 128);
	while (parsePos < input + inputSize)
	{
		/*Next property*/
		memset(defaults, 0, sizeof(float) * 4);
		parsePos = getNextMaterialProperty(parsePos, (input + inputSize) - parsePos, &propType, nameBuff, 512, defaults);
		if (!parsePos)
			break;
		/*Material Members*/
		asShaderFxPropLookup_t propLookup = (asShaderFxPropLookup_t) { 0 };
		uint16_t propOffset = 0;
		propLookup.type = propType;
		if (propType == AS_SHADERFXPROP_SCALAR)
		{
			propOffset = bufferOffset;
			bufferOffset += sizeof(float);
			arrsetlen(ctx->pDefaultBuffer, bufferOffset);
			float* fBuff = (float*)(&ctx->pDefaultBuffer[propOffset]);
			*fBuff = defaults[0];
		}
		else if (propType == AS_SHADERFXPROP_VECTOR4)
		{
			bufferOffset += bufferOffset % (sizeof(float) * 4); /*Enforce vulkan alignment for vectors*/
			propOffset = bufferOffset;
			bufferOffset += sizeof(float) * 4;
			arrsetlen(ctx->pDefaultBuffer, bufferOffset);
			float* fBuff = (float*)(&ctx->pDefaultBuffer[propOffset]);
			fBuff[0] = defaults[0];
			fBuff[1] = defaults[1];
			fBuff[2] = defaults[2];
			fBuff[3] = defaults[3];
		}
		else
		{
			propOffset = textureSlot;
			textureSlot++;
			arrput(ctx->desc.pPropTextureDefaults, (asShaderFxTextureDefault)defaults[0]);
			ctx->desc.propTextureCount++;
		}
		propLookup.nameHash = asHashBytes32_xxHash(nameBuff, strlen(nameBuff));

		arrput(ctx->desc.pProp_Lookup, propLookup);
		arrput(ctx->desc.pProp_Offset, propOffset);
		arrput(ctx->pFxPropNames, strpool_inject(&ctx->stringPool, nameBuff, (int)strlen(nameBuff)));
	}
	ctx->desc.propCount = (uint32_t)arrlen(ctx->desc.pProp_Lookup);
	ctx->desc.propBufferSize = bufferOffset;
}

void fxAssemblerSetupFixedFunctionProps(fxContext_t *ctx, char* input, size_t inputSize)
{
	asCfgFile_t* cfg = (asCfgFile_t*)asCfgFromMem((unsigned char*)input, inputSize);
	const char* tmpStr;
	ctx->fixedFunctions.fillWidth = (float)asCfgGetNumber(cfg, "fillWidth", 1.0f);
	ctx->fixedFunctions.tessCtrlPoints = (uint32_t)asCfgGetNumber(cfg, "tessCtrlPoints", 0.0f);
	tmpStr = asCfgGetString(cfg, "bucket", "DEFAULT");
	ctx->fixedFunctions.bucket = asHashBytes32_xxHash(tmpStr, strlen(tmpStr));
	/*Fill mode*/
	tmpStr = asCfgGetString(cfg, "fillMode", "FULL");
	if (strcmp(tmpStr, "FULL") == 0)
		ctx->fixedFunctions.fillMode = AS_FILL_FULL;
	else if (strcmp(tmpStr, "WIRE") == 0)
		ctx->fixedFunctions.fillMode = AS_FILL_WIRE;
	else if (strcmp(tmpStr, "POINTS") == 0)
		ctx->fixedFunctions.fillMode = AS_FILL_POINTS;
	/*Cull mode*/
	tmpStr = asCfgGetString(cfg, "cullMode", "BACK");
	if (strcmp(tmpStr, "BACK") == 0)
		ctx->fixedFunctions.cullMode = AS_CULL_BACK;
	else if (strcmp(tmpStr, "FRONT") == 0)
		ctx->fixedFunctions.cullMode = AS_CULL_FRONT;
	else if (strcmp(tmpStr, "NONE") == 0)
		ctx->fixedFunctions.cullMode = AS_CULL_NONE;
	/*Blend mode*/
	tmpStr = asCfgGetString(cfg, "blendMode", "NONE");
	if (strcmp(tmpStr, "NONE") == 0)
		ctx->fixedFunctions.blendMode = AS_BLEND_NONE;
	else if (strcmp(tmpStr, "MIX") == 0)
		ctx->fixedFunctions.blendMode = AS_BLEND_MIX;
	else if (strcmp(tmpStr, "ADD") == 0)
		ctx->fixedFunctions.blendMode = AS_BLEND_ADD;
	asCfgFree(cfg);
}

void fxAssemblerAddGeneratorProp(fxContext_t *ctx, char* name, size_t nameSize, char* snippet, size_t snippetSize)
{
	STRPOOL_U64 nameHash = asHashBytes64_xxHash(name, nameSize);
	STRPOOL_U64 valueId = strpool_inject(&ctx->stringPool, snippet, (int)snippetSize);
	arrput(ctx->pGenNameHashes, nameHash);
	arrput(ctx->pGenValues, valueId);
}

void fxAssemblerAddNativeShaderCode(fxContext_t *ctx, asHash32_t nameHash, const char* code, size_t size)
{
	size_t start = arrlen(ctx->desc.pShaderCode);
	arrsetlen(ctx->desc.pShaderCode, start + size);
	memcpy(ctx->desc.pShaderCode + start, code, size);
}