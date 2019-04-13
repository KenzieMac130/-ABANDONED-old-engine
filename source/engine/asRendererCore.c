#include "include/asRendererCore.h"

#include <SDL_video.h>

#if ASTRENGINE_VK
#include "include/lowlevel/asVulkanBackend.h"
#include <SDL_vulkan.h>
#endif

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
		if (strcmp(windowModeStr, "fulscreen") != 0) /*Windowed*/
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

ASEXPORT void asShutdownGfx()
{
#if ASTRENGINE_VK
	asVkShutdown();
#endif
	SDL_DestroyWindow(asMainWindow);
}

ASEXPORT uint32_t asTextureCalcPitch(asTextureFormat format, uint32_t width)
{
	uint32_t tmpVal = 0;
	switch (format)
	{
		case AS_TEXTUREFORMAT_BC1_UNORM_BLOCK:
			tmpVal = ((width + 3) / 4) * 8;
			return tmpVal < 8 ? 8 : tmpVal;
			break;
		case AS_TEXTUREFORMAT_BC3_UNORM_BLOCK:
		case AS_TEXTUREFORMAT_BC5_UNORM_BLOCK:
		case AS_TEXTUREFORMAT_BC6H_UFLOAT_BLOCK:
		case AS_TEXTUREFORMAT_BC7_UNORM_BLOCK:
			tmpVal = ((width + 3) / 4) * 16;
			return tmpVal < 16 ? 16 : tmpVal;
			break;
		default:
			switch (format)
			{
			case AS_TEXTUREFORMAT_RGBA8_UNORM:
			case AS_TEXTUREFORMAT_R16G16_SFLOAT:
			case AS_TEXTUREFORMAT_R32_SFLOAT:
			case AS_TEXTUREFORMAT_R10G10B10A2_UNORM:
				tmpVal = 4;
				break;
			case AS_TEXTUREFORMAT_RGBA16_UNORM:
			case AS_TEXTUREFORMAT_RGBA16_SFLOAT:
				tmpVal = 8;
				break;
			case AS_TEXTUREFORMAT_R16_SFLOAT:
				tmpVal = 2;
				break;
			case AS_TEXTUREFORMAT_R8_UNORM:
				tmpVal = 1;
				break;
			case AS_TEXTUREFORMAT_DEPTH:
				asGetDepthFormatSize(format);
			default:
				break;
			}
			return (width * tmpVal + 7) / 8;
		break;
	}
	return 0;
}

ASEXPORT asTextureDesc_t asTextureDesc_Init()
{
	asTextureDesc_t result = (asTextureDesc_t) { 0 };
	result.depth = 1;
	result.mips = 1;
	result.maxLod = FLT_MAX;
	result.maxAnisotropy = 16;
	result.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
	return result;
}