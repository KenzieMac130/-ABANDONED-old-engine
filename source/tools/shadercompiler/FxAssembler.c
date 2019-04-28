#include "FxAssembler.h"

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"

struct fxContext_t
{
	asShaderFxDesc_t desc;
};

fxContext_t* fxAssemblerInit()
{
	fxContext_t *ctx = asMalloc(sizeof(fxContext_t));
	ctx->desc = asShaderFxDesc_Init();
	return ctx;
}

void fxAssemblerRelease(fxContext_t *ctx)
{
	arrfree(ctx->desc.pBindings);
	for (uint32_t i = 0; i < ctx->desc.bufferDescCount; i++)
		arrfree(ctx->desc.pBufferDescs[i].pMembers);
	arrfree(ctx->desc.pBufferDescs);
	arrfree(ctx->desc.pMaterialDefaults);
	arrfree(ctx->desc.pSamplerDescs);
	arrfree(ctx->desc.pShaderPrograms);
	arrfree(ctx->desc.pVertexInputs);
	asFree(ctx);
}

/*Material Props*/

char* getNextMaterialProperty(char* input, size_t inputSize,
	asShaderFxBindingType *outBindType,
	asShaderFxVarType *outVarType,
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
		*outBindType = -1;
		*outVarType = AS_SHADERVARTYPE_FLOAT;
	}
	else if (strcmp(tmpBuff, "Vec4") == 0){
		/*Type is vector4*/
		*outBindType = -1;
		*outVarType = AS_SHADERVARTYPE_FVECTOR4;
	}
	else if (strcmp(tmpBuff, "Tex2D") == 0){
		/*Type is Texture 2D*/
		*outBindType = AS_SHADERBINDING_TEX2D;
	}
	else if (strcmp(tmpBuff, "Tex2DArray") == 0){
		/*Type is Texture 2D*/
		*outBindType = AS_SHADERBINDING_TEX2DARRAY;
	}
	else if (strcmp(tmpBuff, "TexCube") == 0){
		/*Type is Texture 2D*/
		*outBindType = AS_SHADERBINDING_TEXCUBE;
	}
	else if (strcmp(tmpBuff, "TexCubeArray") == 0){
		/*Type is Texture 2D*/
		*outBindType = AS_SHADERBINDING_TEXCUBEARRAY;
	}
	else if (strcmp(tmpBuff, "Tex3D") == 0) {
		/*Type is Texture 2D*/
		*outBindType = AS_SHADERBINDING_TEX3D;
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
	switch (*outBindType)
	{
	/*Properties*/
	case -1:
		if (*outVarType == AS_SHADERVARTYPE_FLOAT){
			outDefaults[0] = strtof(defaultStartPos, &defaultEndPos);
		}
		else if (*outVarType == AS_SHADERVARTYPE_FVECTOR4){
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
	asShaderFxBufferLayoutDesc_t bufferLayout = (asShaderFxBufferLayoutDesc_t) { 0 };
	bufferLayout.nameHash = asHashBytes64_xxHash("Material", strlen("Material"));

	/*Parse each material property*/
	asShaderFxBindingType bindType = (asShaderFxBindingType)-1;
	asShaderFxVarType varType = (asShaderFxVarType)-1;
	char nameBuff[512];
	float defaults[4];
	asShaderFxBufferMemberDesc_t member;
	asShaderFxMaterialDefault_t matDefault;
	asShaderFxBindingDesc_t binding;
	uint32_t textureSlot = 1; /*Start at 1 to make room for the material*/
	char* parsePos = input;
	while (parsePos < input + inputSize)
	{
		/*Next property*/
		memset(defaults, 0, sizeof(float) * 4);
		parsePos = getNextMaterialProperty(parsePos, (input + inputSize) - parsePos, &bindType, &varType, nameBuff, 512, defaults);
		if (parsePos == NULL)
			break;
		/*Material Members*/
		if (bindType == -1) 
		{
			member = (asShaderFxBufferMemberDesc_t) { 0 };
			member.nameHash = asHashBytes64_xxHash(nameBuff, strlen(nameBuff));
			member.type = varType;
			arrput(bufferLayout.pMembers, member);
			bufferLayout.memberCount++;
		}
		/*Textures*/
		else 
		{
			binding = (asShaderFxBindingDesc_t) { 0 };
			binding.nameHash = asHashBytes64_xxHash(nameBuff, strlen(nameBuff));
			binding.slot = textureSlot;
			binding.type = bindType;
			arrput(ctx->desc.pBindings, binding);
			ctx->desc.bindingCount++;
			textureSlot++;
		}
		/*Write material default*/
		matDefault = (asShaderFxMaterialDefault_t) { 0 };
		matDefault.nameHash = asHashBytes64_xxHash(nameBuff, strlen(nameBuff));
		matDefault.values[0] = defaults[0];
		matDefault.values[1] = defaults[1];
		matDefault.values[2] = defaults[2];
		matDefault.values[3] = defaults[3];
		arrput(ctx->desc.pMaterialDefaults, matDefault);
		ctx->desc.materialDefaultCount++;
	}

	/*Add Material Uniform Buffer*/
	if (bufferLayout.memberCount)
	{
		binding = (asShaderFxBindingDesc_t) { 0 };
		binding.nameHash = bufferLayout.nameHash;
		binding.slot = 0;
		binding.type = AS_SHADERBINDING_UBO;
		arrput(ctx->desc.pBindings, binding);
		ctx->desc.bindingCount++;
		arrput(ctx->desc.pBufferDescs, bufferLayout);
		ctx->desc.bufferDescCount++;
	}
}

void fxAssemblerSetupFixedFunctionProps(fxContext_t *ctx, char* input, size_t inputSize)
{
	asCfgFile_t* cfg = asCfgFromMem(input, inputSize);
	const char* tmpStr;
	ctx->desc.fixedFunctions.fillWidth = asCfgGetNumber(cfg, "fillWidth", 1.0f);
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
		ctx->desc.fixedFunctions.fillMode = AS_CULL_FRONT;
	else if (strcmp(tmpStr, "NONE") == 0)
		ctx->desc.fixedFunctions.fillMode = AS_CULL_NONE;

	char tmpBuff[64];
	for (int i = 0; i < AS_MAX_PIXELOUTPUTS; i++)
	{
		snprintf(tmpBuff, 64, "blend%d\0", i);
		asCfgOpenSection(cfg, tmpBuff);
		{
			ctx->desc.fixedFunctions.colorBlends[i].enabled = asCfgGetNumber(cfg, "enabled", 0) ? true : false;
			//ctx->desc.fixedFunctions.colorBlends[i].
		}
	}
	
	/*Cull mode*/
	asCfgFree(cfg);
}