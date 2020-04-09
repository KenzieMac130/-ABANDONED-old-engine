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

#include "../common/preferences/asPreferences.h"
#include "asBindlessTexturePool.h"

asLinearMemoryAllocator_t* pCurrentLinearAllocator;

ASEXPORT uint32_t asTextureCalcPitch(asColorFormat format, uint32_t width)
{
	uint32_t tmpVal = 0;
	switch (format)
	{
	case AS_COLORFORMAT_BC1_RGBA_UNORM_BLOCK:
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
		case AS_COLORFORMAT_A2R10G10B10_UNORM:
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

/*Settings*/
static int32_t gfxSettingsMonitor = 0;
static int32_t gfxSettingsWinMode = 0;
static int32_t gfxSettingsWinWidth = 1280;
static int32_t gfxSettingsWinHeight = 720;
static int32_t gfxSettingsDevice = -1;

void* gpCustomWindow;
asAppInfo_t* gpAppInfo;
void createWindow(asAppInfo_t* pAppInfo, void* pCustomWindow)
{
	/*Window Creation*/
	if (pCustomWindow)
	{
		asMainWindow = SDL_CreateWindowFrom(pCustomWindow);
	}
	else
	{
		int monitor = gfxSettingsMonitor;
		SDL_Rect windowDim;
		SDL_GetDisplayBounds(monitor, &windowDim);
		uint32_t windowFlags = 0;
#if ASTRENGINE_VK
		windowFlags |= SDL_WINDOW_VULKAN;
#endif
		if (gfxSettingsWinMode == 0 || gfxSettingsWinMode == 1) /*Windowed*/
		{
			windowDim.x = SDL_WINDOWPOS_CENTERED_DISPLAY(monitor);
			windowDim.y = SDL_WINDOWPOS_CENTERED_DISPLAY(monitor);
			windowDim.w = gfxSettingsWinWidth;
			windowDim.h = gfxSettingsWinHeight;

			if (gfxSettingsWinMode == 1)
			{
				windowFlags |= SDL_WINDOW_RESIZABLE;
			}
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
		asFatalError("Failed to find window");
	}
}

asResults _applySettings(const char* propName, void* pCurrentValue, void* pNewValueTmp, void* pUserData)
{
	SDL_DestroyWindow(asMainWindow);
	createWindow(gpAppInfo, gpCustomWindow);
	asGfxTriggerResizeEvent();
	return AS_SUCCESS;
}

ASEXPORT void asInitGfx(asAppInfo_t *pAppInfo, void* pCustomWindow)
{
	gpCustomWindow = pCustomWindow;
	gpAppInfo = pAppInfo;
	/*Setup Config Variables*/
	asPreferencesRegisterOpenSection(asGetGlobalPrefs(), "gfx");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "monitor", &gfxSettingsMonitor, 0, 16, true, NULL, NULL, 
		"Index of the monitor to spawn the window on");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "windowMode", &gfxSettingsWinMode, 0, 2, true, NULL, NULL,
		"Window mode (0: Windowed, 1: Resizable Windowed, 2: Borderless Fullscreen)");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "windowWidth", &gfxSettingsWinWidth, 640, 65536, true, NULL, NULL,
		"Window hidth");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "windowHeight", &gfxSettingsWinHeight, 480, 65536, true, NULL, NULL,
		"Window height");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "deviceIndex", &gfxSettingsDevice, -1, 64, true, NULL, NULL,
		"Render device index (GPU) (-1: Autoselect)");
	asPreferencesRegisterNullFunction(asGetGlobalPrefs(), "applySettings", _applySettings, NULL,
		"Reinitialize the implimentations as necessary for Settings");
	asPreferencesLoadSection(asGetGlobalPrefs(), "gfx");

	createWindow(gpAppInfo, gpCustomWindow);

#if ASTRENGINE_VK
	asVkInit(pAppInfo, gfxSettingsDevice);
#endif
	asInitTexturePool();
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

ASEXPORT void asGfxInternalDebugDraws()
{
	_asTexturePoolDebug();
}

ASEXPORT void asGfxRenderFrame()
{
	if (_frameSkip) /*Frame Skip*/
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
	asTexturePoolUpdate();
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
	asShutdownTexturePool();
	asVkFinalShutdown();
#endif
	SDL_DestroyWindow(asMainWindow);
}