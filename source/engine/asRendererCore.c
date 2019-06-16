#include "include/asRendererCore.h"

#include <SDL_video.h>

#if ASTRENGINE_VK
#include "include/lowlevel/asVulkanBackend.h"
#include <SDL_vulkan.h>
#endif

#if ASTRENGINE_NUKLEAR
#include "include/asNuklearImplimentation.h"
#endif

ASEXPORT uint32_t asTextureCalcPitch(asColorFormat format, uint32_t width)
{
	uint32_t tmpVal = 0;
	switch (format)
	{
	case AS_COLORFORMAT_BC1_UNORM_BLOCK:
		tmpVal = ((width + 3) / 4) * 8;
		return tmpVal < 8 ? 8 : tmpVal;
		break;
	case AS_COLORFORMAT_BC3_UNORM_BLOCK:
	case AS_COLORFORMAT_BC5_UNORM_BLOCK:
	case AS_COLORFORMAT_BC6H_UFLOAT_BLOCK:
	case AS_COLORFORMAT_BC7_UNORM_BLOCK:
		tmpVal = ((width + 3) / 4) * 16;
		return tmpVal < 16 ? 16 : tmpVal;
		break;
	default:
		switch (format)
		{
		case AS_COLORFORMAT_RGBA8_UNORM:
		case AS_COLORFORMAT_RG16_SFLOAT:
		case AS_COLORFORMAT_R32_SFLOAT:
		case AS_COLORFORMAT_R10G10B10A2_UNORM:
		case AS_COLORFORMAT_B10G11R11_UFLOAT:
			tmpVal = 4;
			break;
		case AS_COLORFORMAT_RGB16_SFLOAT:
			tmpVal = 6;
			break;
		case AS_COLORFORMAT_RGBA16_UNORM:
		case AS_COLORFORMAT_RGBA16_SFLOAT:
			tmpVal = 8;
			break;
		case AS_COLORFORMAT_RGBA32_SFLOAT:
		case AS_COLORFORMAT_RGBA32_UINT:
			tmpVal = 16;
			break;
		case AS_COLORFORMAT_RGB32_SFLOAT:
			tmpVal = 12;
			break;
		case AS_COLORFORMAT_R16_SFLOAT:
			tmpVal = 2;
			break;
		case AS_COLORFORMAT_R8_UNORM:
			tmpVal = 1;
			break;
		case AS_COLORFORMAT_DEPTH:
			asGetDepthFormatSize(format);
		default:
			break;
		}
		return (width * tmpVal + 7) / 8;
		break;
	}
	return 0;
}

struct _fxHeader
{
	struct validation
	{
		char name[4];
		uint8_t endian;
		uint8_t api;
		uint16_t apiVersion;
		uint32_t version;
	} valid;
	uint32_t buffersize;
};

#define FXHEADERDEFAULT {"ASFX", AS_ENDIAN, AS_GFXAPI, AS_GFXAPI_VERSION, AS_SHADERFX_VERSION}

ASEXPORT int32_t asShaderFxDesc_SaveToFile(const char* fileName, asShaderFxDesc_t *fx, asGfxAPIs api, uint32_t apiVersion)
{
	FILE* fp = fopen(fileName, "wb");
	if (!fp)
		return -1;

	struct _fxHeader header = FXHEADERDEFAULT;
	header.valid.api = api;
	header.valid.apiVersion = apiVersion;
	header.buffersize =
		sizeof(asShaderFxPropLookup_t) * fx->propCount +
		sizeof(uint16_t) * fx->propCount +
		sizeof(asShaderFxProgramLookup_t) * fx->programCount +
		sizeof(asShaderFxProgramDesc_t) * fx->programCount +
		fx->shaderCodeSize + fx->propBufferSize;

	fwrite(&header, sizeof(struct _fxHeader), 1, fp);
	fwrite(fx, 4 * 16, 1, fp);

	fwrite(fx->pPropLookup, sizeof(asShaderFxPropLookup_t) * fx->propCount, 1, fp);
	fwrite(fx->pPropOffset, sizeof(uint16_t) * fx->propCount, 1, fp);

	fwrite(fx->pProgramLookup, sizeof(asShaderFxProgramLookup_t) * fx->programCount, 1, fp);
	fwrite(fx->pProgramDescs, sizeof(asShaderFxProgramDesc_t) * fx->programCount, 1, fp);

	fwrite(fx->pShaderCode, fx->shaderCodeSize, 1, fp);
	fwrite(fx->pPropBufferDefault, fx->propBufferSize, 1, fp);

	fclose(fp);
	return 0;
}

ASEXPORT int32_t asShaderFxDesc_PeekFile(const char* fileName, asShaderFx_FileContext_t *ctx)
{
	ctx->fp = fopen(fileName, "rb");
	if (!ctx->fp)
		return 0;
	struct _fxHeader test = FXHEADERDEFAULT;
	struct _fxHeader header;
	fread(&header, sizeof(struct _fxHeader), 1, ctx->fp);
	if (memcmp(&test.valid, &header.valid, sizeof(header.valid)))
	{
		fclose(ctx->fp);
		return 0;
	}
	return (int32_t)header.buffersize;
}

ASEXPORT asShaderFxDesc_t asShaderFxDesc_ReadFile(unsigned char* outBuffer, size_t outBufferSize, asShaderFx_FileContext_t *ctx)
{
	asShaderFxDesc_t result;
	fread(&result, 4 * 16, 1, ctx->fp);
	fread(outBuffer, outBufferSize, 1, ctx->fp);

	/*offset pointers into buffers*/
	result.pPropLookup = outBuffer;
	result.pPropOffset = outBuffer += sizeof(result.pPropLookup[0]) * result.propCount;
	result.pProgramLookup = outBuffer += sizeof(result.pPropOffset[0]) * result.propCount;
	result.pProgramDescs = outBuffer += sizeof(result.pProgramLookup[0]) * result.programCount;
	result.pShaderCode = outBuffer += sizeof(result.pProgramDescs[0]) * result.programCount;
	result.pPropBufferDefault = outBuffer += result.shaderCodeSize;

	fclose(ctx->fp);
	return result;
}


ASEXPORT asTextureDesc_t asTextureDesc_Init()
{
	asTextureDesc_t result = (asTextureDesc_t) { 0 };
	result.depth = 1;
	result.mips = 1;
	result.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
	return result;
}

ASEXPORT asBufferDesc_t asBufferDesc_Init()
{
	asBufferDesc_t result = (asBufferDesc_t) { 0 };
	return result;
}

ASEXPORT asShaderFxDesc_t asShaderFxDesc_Init()
{
	asShaderFxDesc_t result = (asShaderFxDesc_t) { 0 };
	return result;
}

SDL_Window *asMainWindow;
ASEXPORT SDL_Window* asGetMainWindowPtr()
{
	return asMainWindow;
}

ASEXPORT void asInitGfx(asAppInfo_t *pAppInfo, void* pCustomWindow)
{
	/*Read Config File*/
	asCfgFile_t *pConfig = asCfgLoad(pAppInfo->pGfxIniName);

	/*Window Creation*/
	if (pCustomWindow)
	{
		asMainWindow = SDL_CreateWindowFrom(pCustomWindow);
	}
	else
	{
		int monitor = (int)asCfgGetNumber(pConfig, "Monitor", 0);
		SDL_Rect windowDim; 
		SDL_GetDisplayBounds(monitor, &windowDim);
		uint32_t windowFlags = 0;
#if ASTRENGINE_VK
		windowFlags |= SDL_WINDOW_VULKAN;
#endif
		const char* windowModeStr = asCfgGetString(pConfig, "WindowMode", "windowed");
		if ((strcmp(windowModeStr, "fullscreen") != 0)) /*Windowed*/
		{
			windowDim.x = SDL_WINDOWPOS_CENTERED_DISPLAY(monitor);
			windowDim.y = SDL_WINDOWPOS_CENTERED_DISPLAY(monitor);
			windowDim.w = (int)asCfgGetNumber(pConfig, "Width", 640);
			windowDim.h = (int)asCfgGetNumber(pConfig, "Height", 480);
			
			if(strcmp(windowModeStr, "resizable") == 0)
				windowFlags |= SDL_WINDOW_RESIZABLE;
		}
		else /*Borderless Fullscreen*/
		{
			windowFlags |= SDL_WINDOW_BORDERLESS;
		}
		asMainWindow = SDL_CreateWindow(pAppInfo->pAppName,
			windowDim.x,
			windowDim.y,
			windowDim.w,
			windowDim.h,
			windowFlags);
		SDL_SetWindowMinimumSize(asMainWindow, 640, 480);
	}
	if (!asMainWindow)
	{
		asCfgFree(pConfig);
		asFatalError("Failed to find window");
	}

#if ASTRENGINE_VK
	asVkInit(pAppInfo, pConfig);
#endif

	asCfgFree(pConfig);
}

bool _frameSkip = false;
ASEXPORT void asGfxSetDrawSkip(bool skip)
{
	_frameSkip = skip;
}

ASEXPORT void asGfxTriggerResizeEvent()
{
	if (_frameSkip)
		return;
#if ASTRENGINE_VK
	asVkWindowResize();
#endif
}

ASEXPORT void asGfxRenderFrame()
{
	if (_frameSkip)
	{
		asNkReset();
		return;
	}
#if ASTRENGINE_NUKLEAR
	asNkDraw();
#endif
#if ASTRENGINE_VK
	asVkDrawFrame();
#endif
}

ASEXPORT void asShutdownGfx()
{
#if ASTRENGINE_VK
	asVkShutdown();
#endif
	SDL_DestroyWindow(asMainWindow);
}