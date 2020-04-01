#include "asRendererCore.h"

#include <SDL_video.h>

#if ASTRENGINE_VK
#include "vulkan/asVulkanBackend.h"
#include <SDL_vulkan.h>
#endif

#if ASTRENGINE_NUKLEAR
#include "../nuklear/asNuklearImplimentation.h"
#endif
#if ASTRENGINE_DEARIMGUI
#include "../cimgui/asDearImGuiImplimentation.h"
#endif

asLinearMemoryAllocator_t* pCurrentLinearAllocator;

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

/*Texture*/

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

SDL_Window *asMainWindow;
ASEXPORT SDL_Window* asGetMainWindowPtr()
{
	return asMainWindow;
}

ASEXPORT void asInitGfx(asAppInfo_t *pAppInfo, void* pCustomWindow)
{
	/*Read Config File*/
	asCfgFile_t *pConfig = asCfgLoadUserFile("graphics.ini");

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
		if ((asIsStringEqual(windowModeStr, "fullscreen"))) /*Windowed*/
		{
			windowDim.x = SDL_WINDOWPOS_CENTERED_DISPLAY(monitor);
			windowDim.y = SDL_WINDOWPOS_CENTERED_DISPLAY(monitor);
			windowDim.w = (int)asCfgGetNumber(pConfig, "Width", 640);
			windowDim.h = (int)asCfgGetNumber(pConfig, "Height", 480);
			
			if(asIsStringEqual(windowModeStr, "resizable"))
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
	asVkWindowResize(pCurrentLinearAllocator);
#endif
}

ASEXPORT void asGfxRenderFrame()
{
	if (_frameSkip)
	{
#if ASTRENGINE_NUKLEAR
		asNkReset();
#endif
#if ASTRENGINE_DEARIMGUI
		asImGuiReset();
#endif
		return;
	}
#if ASTRENGINE_VK
	asVkInitFrame();
#endif
#if ASTRENGINE_NUKLEAR
	asNkDraw(0);
#endif
#if ASTRENGINE_DEARIMGUI
	asImGuiDraw(0);
#endif
#if ASTRENGINE_VK
	asVkDrawFrame();
#endif
}

ASEXPORT void asShutdownGfx()
{
#if ASTRENGINE_VK
	asVkInitShutdown();
#if ASTRENGINE_NUKLEAR
	asShutdownGfxNk();
#endif
#if ASTRENGINE_DEARIMGUI
	asShutdownGfxImGui();
#endif
	asVkFinalShutdown();
#endif
	SDL_DestroyWindow(asMainWindow);
}