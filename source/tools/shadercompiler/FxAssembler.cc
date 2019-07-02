#include "FxAssembler.h"
#include "stb/stb_ds.h"

cFxContext::cFxContext(std::ostringstream *debugStream)
{
	this->pDebugStream = debugStream;
	this->desc = asShaderFxDesc_Init();
	strpool_config_t conf = strpool_default_config;
	arrsetcap(this->desc.pShaderCode, 1024);
}

cFxContext::~cFxContext()
{
	arrfree(this->desc.pPropLookup);
	arrfree(this->desc.pPropOffset);
	arrfree(this->desc.pProgramLookup);
	arrfree(this->desc.pProgramDescs);
	arrfree(this->desc.pShaderCode);
	arrfree(this->desc.pPropBufferDefault);
}

/*Material Props*/

char* getNextMaterialProperty(cFxContext* ctx, char* input, size_t inputSize,
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
		if (ctx->pDebugStream)
			*ctx->pDebugStream << "Type: " << tmpBuff << " Not Found\n";
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

void cFxContext::fxAssemblerSetupMaterialProps(char* input, size_t inputSize)
{
	cFxContext* ctx = this;
	/*Parse each material property*/
	arrsetcap(ctx->desc.pPropBufferDefault, 256);

	struct _tmpProp {
		asShaderFxPropLookup_t lookup;
		float defaults[4];
	};
	/*one pool for floats one for vec4 one for all textures*/
	std::vector<_tmpProp> tmpProps[3];
	std::vector<std::string> tmpPropNames[3];
	for (int i = 0; i < 3; i++){
		tmpProps[i].reserve(32);
		tmpPropNames[i].reserve(32);
	}

	/*Parse the string*/
	{
		std::string tmpString;
		tmpString.reserve(512);
		char nameBuff[512];
		float defaults[4];
		char* parsePos = input;
		while (parsePos < input + inputSize)
		{
			asShaderFxPropType propType = (asShaderFxPropType)-1;
			/*Next property*/
			memset(defaults, 0, sizeof(float) * 4);
			parsePos = getNextMaterialProperty(ctx, parsePos, (input + inputSize) - parsePos, &propType, nameBuff, 512, defaults);
			if (!parsePos)
				break;
			/*Material Members*/
			_tmpProp prop;
			prop.lookup.nameHash = asHashBytes32_xxHash(nameBuff, strlen(nameBuff));
			prop.lookup.type = propType;
			prop.defaults[0] = defaults[0];
			prop.defaults[1] = defaults[1];
			prop.defaults[2] = defaults[2];
			prop.defaults[3] = defaults[3];

			int pool = propType > 2 ? 2 : propType;
			tmpProps[pool].push_back(prop);
			tmpPropNames[pool].push_back(nameBuff);

			/*Texture coordinate stuff*/
			/*
			if (propType == AS_SHADERFXPROP_TEX2D)
			{
				prop.lookup.type = AS_SHADERFXPROP_INTEGER;
				prop.defaults[0] = 0.0f;
				tmpString = nameBuff;
				tmpString.append("_uv");
				prop.lookup.nameHash = asHashBytes32_xxHash(tmpString.c_str(), tmpString.length());
				tmpProps[AS_SHADERFXPROP_INTEGER].push_back(prop);
				tmpPropNames[AS_SHADERFXPROP_INTEGER].push_back(tmpString);

				prop.lookup.type = AS_SHADERFXPROP_VECTOR4;
				prop.defaults[0] = 0.0f;
				prop.defaults[1] = 0.0f;
				prop.defaults[2] = 1.0f;
				prop.defaults[3] = 1.0f;
				tmpString = nameBuff;
				tmpString.append("_xform");
				prop.lookup.nameHash = asHashBytes32_xxHash(tmpString.c_str(), tmpString.length());
				tmpProps[AS_SHADERFXPROP_VECTOR4].push_back(prop);
				tmpPropNames[AS_SHADERFXPROP_VECTOR4].push_back(tmpString);
			}
			*/
		}
	}

	/*add all the properties*/
	uint32_t bufferOffset = 0;
	uint16_t propOffset = 0;
	uint32_t textureSlot = 1; /*Start at 1 to make room for the material buffer*/

	/*Check for overflow (enforce limits for future buffer use)*/
	if (sizeof(float)*tmpProps[AS_SHADERFXPROP_SCALAR].size() +
		sizeof(int32_t)*tmpProps[AS_SHADERFXPROP_INTEGER].size() +
		sizeof(float) * 4 * tmpProps[AS_SHADERFXPROP_VECTOR4].size() > 
		AS_MATERIAL_BUFFER_SIZE)
	{
		*this->pDebugStream << "MATERIAL BUFFER OVERFLOW! (lower property count)\n";
		return;
	}

	/*Vector4s*/
	for (size_t i = 0; i < tmpProps[AS_SHADERFXPROP_VECTOR4].size(); i++)
	{
		_tmpProp tmpProp = tmpProps[AS_SHADERFXPROP_VECTOR4][i];
		bufferOffset += bufferOffset % (sizeof(float) * 4); /*Enforce vulkan alignment for vectors*/
		propOffset = (uint16_t)bufferOffset;
		bufferOffset += sizeof(float) * 4;
		arrsetlen(ctx->desc.pPropBufferDefault, bufferOffset);
		float* fBuff = (float*)(&ctx->desc.pPropBufferDefault[propOffset]);
		fBuff[0] = tmpProp.defaults[0];
		fBuff[1] = tmpProp.defaults[1];
		fBuff[2] = tmpProp.defaults[2];
		fBuff[3] = tmpProp.defaults[3];

		ctx->FxPropNames.push_back(tmpPropNames[AS_SHADERFXPROP_VECTOR4][i]);
		arrput(ctx->desc.pPropLookup, tmpProp.lookup);
		arrput(ctx->desc.pPropOffset, propOffset);
	}
	/*Scalars*/
	for (size_t i = 0; i < tmpProps[AS_SHADERFXPROP_SCALAR].size(); i++)
	{
		_tmpProp tmpProp = tmpProps[AS_SHADERFXPROP_SCALAR][i];
		propOffset = (uint16_t)bufferOffset;
		bufferOffset += sizeof(float);
		arrsetlen(ctx->desc.pPropBufferDefault, bufferOffset);
		float* fBuff = (float*)(&ctx->desc.pPropBufferDefault[propOffset]);
		*fBuff = tmpProp.defaults[0];

		ctx->FxPropNames.push_back(tmpPropNames[AS_SHADERFXPROP_SCALAR][i]);
		arrput(ctx->desc.pPropLookup, tmpProp.lookup);
		arrput(ctx->desc.pPropOffset, propOffset);
	}
	/*Integers*/
	for (size_t i = 0; i < tmpProps[AS_SHADERFXPROP_INTEGER].size(); i++)
	{
		_tmpProp tmpProp = tmpProps[AS_SHADERFXPROP_INTEGER][i];
		propOffset = (uint16_t)bufferOffset;
		bufferOffset += sizeof(uint32_t);
		arrsetlen(ctx->desc.pPropBufferDefault, bufferOffset);
		uint32_t* fBuff = (uint32_t*)(&ctx->desc.pPropBufferDefault[propOffset]);
		*fBuff = (uint32_t)tmpProp.defaults[0];

		ctx->FxPropNames.push_back(tmpPropNames[AS_SHADERFXPROP_INTEGER][i]);
		arrput(ctx->desc.pPropLookup, tmpProp.lookup);
		arrput(ctx->desc.pPropOffset, propOffset);
	}
	/*Padding*/
	{
		defaultBufferPadding = AS_MATERIAL_BUFFER_SIZE - bufferOffset;
		arrsetlen(ctx->desc.pPropBufferDefault, AS_MATERIAL_BUFFER_SIZE);
		memset(ctx->desc.pPropBufferDefault + bufferOffset, 0, defaultBufferPadding);
		bufferOffset = AS_MATERIAL_BUFFER_SIZE;
	}

	ctx->desc.propCount = (uint32_t)arrlen(ctx->desc.pPropLookup);
	ctx->desc.propBufferSize = bufferOffset;
}

void cFxContext::fxAssemblerSetupFixedFunctionProps(char* input, size_t inputSize)
{
	cFxContext* ctx = this;
	asCfgFile_t* cfg = (asCfgFile_t*)asCfgFromMem((unsigned char*)input, inputSize);
	const char* tmpStr;
	ctx->desc.fillSize = (float)asCfgGetNumber(cfg, "fillSize", 1.0f);
	ctx->desc.tessCtrlPoints = (uint32_t)asCfgGetNumber(cfg, "tessCtrlPoints", 0.0f);
	tmpStr = asCfgGetString(cfg, "bucket", "DEFAULT");
	ctx->desc.bucket = asHashBytes32_xxHash(tmpStr, strlen(tmpStr));
	/*Fill mode*/
	tmpStr = asCfgGetString(cfg, "fillMode", "FULL");
	if (strcmp(tmpStr, "FULL") == 0)
		ctx->desc.fillMode = AS_FILL_FULL;
	else if (strcmp(tmpStr, "WIRE") == 0)
		ctx->desc.fillMode = AS_FILL_WIRE;
	else if (strcmp(tmpStr, "POINTS") == 0)
		ctx->desc.fillMode = AS_FILL_POINTS;
	/*Cull mode*/
	tmpStr = asCfgGetString(cfg, "cullMode", "BACK");
	if (strcmp(tmpStr, "BACK") == 0)
		ctx->desc.cullMode = AS_CULL_BACK;
	else if (strcmp(tmpStr, "FRONT") == 0)
		ctx->desc.cullMode = AS_CULL_FRONT;
	else if (strcmp(tmpStr, "NONE") == 0)
		ctx->desc.cullMode = AS_CULL_NONE;
	/*Blend mode*/
	tmpStr = asCfgGetString(cfg, "blendMode", "NONE");
	if (strcmp(tmpStr, "NONE") == 0)
		ctx->desc.blendMode = AS_BLEND_NONE;
	else if (strcmp(tmpStr, "MIX") == 0)
		ctx->desc.blendMode = AS_BLEND_MIX;
	else if (strcmp(tmpStr, "ADD") == 0)
		ctx->desc.blendMode = AS_BLEND_ADD;
	asCfgFree(cfg);
}

void cFxContext::fxAssemblerAddGeneratorProp(char* name, size_t nameSize, char* snippet, size_t snippetSize)
{
	this->GenNameHashes.push_back(asHashBytes64_xxHash(name, nameSize));
	this->GenValues.push_back(std::string(snippet, snippet+ snippetSize));
}

void cFxContext::fxAssemblerAddNativeShaderCode(asHash32_t nameHash, asShaderStage stage, const char* code, size_t size)
{
	cFxContext* ctx = this;
	size_t start = arrlen(ctx->desc.pShaderCode);
	arrsetlen(ctx->desc.pShaderCode, start + size);
	memcpy(ctx->desc.pShaderCode + start, code, size);
	ctx->desc.shaderCodeSize += (uint32_t)size;
	
	asShaderFxProgramLookup_t lookup;
	lookup.nameHash = nameHash;
	lookup.stage = stage;
	arrput(ctx->desc.pProgramLookup, lookup);

	asShaderFxProgramDesc_t desc;
	desc.programByteStart = (uint32_t)start;
	desc.programByteCount = (uint32_t)size;
	arrput(ctx->desc.pProgramDescs, desc);

	ctx->desc.programCount++;
}