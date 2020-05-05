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
		case AS_COLORFORMAT_A2R10G10B10_SNORM:
		case AS_COLORFORMAT_B10G11R11_UFLOAT:
			tmpVal = 4;
			break;
		case AS_COLORFORMAT_RGB16_SFLOAT:
			tmpVal = 6;
			break;
		case AS_COLORFORMAT_RGBA16_UNORM:
		case AS_COLORFORMAT_RGBA16_UINT:
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
			asFatalError("Unknown image format used");
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

/*Shader FX*/
asResourceType_t shaderFxResourceType;
ASEXPORT asShaderFx* asShaderFxManagerGetShaderFx(asResourceFileID_t resourceFileId)
{
	/*Get Existing*/
	asResourceDataMapping_t dataMap = asResource_GetExistingDataMapping(resourceFileId, shaderFxResourceType);
	if (dataMap.ptr != NULL) {
		asResource_IncrimentReferences(resourceFileId, 1);
		return dataMap.ptr;
	}

	/*Create if Not Existing*/
	asShaderFx* pFx = asMalloc(sizeof(asShaderFx));
	memset(pFx, 0, sizeof(asShaderFx));

	unsigned char* fileData;
	size_t fileSize;
	asResourceLoader_t resourceLoader;
	if (asResourceLoader_Open(&resourceLoader, resourceFileId) != AS_SUCCESS) { return NULL; }
	fileSize = asResourceLoader_GetContentSize(&resourceLoader);
	fileData = asMalloc(fileSize);
	asResourceLoader_ReadAll(&resourceLoader, fileSize, fileData);
	asResourceLoader_Close(&resourceLoader);

	asBinReader shaderBin;
	if (asBinReaderOpenMemory(&shaderBin, "ASFX", fileData, fileSize) != AS_SUCCESS)
	{
		asDebugError("ShaderFX file Corrupted!");
		return NULL;
	}
	if (asCreateShaderFx(&shaderBin, pFx, AS_QUALITY_HIGH) != AS_SUCCESS)
	{
		asDebugError("Could not load from ShaderFx database!");
		return NULL;
	}
	asFree(fileData);

	/*Create Mappings*/
	dataMap.ptr = pFx;
	asResource_Create(resourceFileId, dataMap, shaderFxResourceType, 1);
	return pFx;
}

ASEXPORT void asShaderFxManagerDereferenceShaderFx(asResourceFileID_t resourceFileId)
{
	asResource_DeincrimentReferences(resourceFileId, 1);
}

void _ShaderFxManagerInit()
{
	shaderFxResourceType = asResource_RegisterType("ASFX", 4);
}

void _ShaderFxGC()
{
	size_t count;
	asResourceDataMapping_t* pToDelete;
	asResource_GetDeletionQueue(shaderFxResourceType, &count, &pToDelete);
	if (count > 0)
	{
		for (size_t i = 0; i < count; i++)
		{
			asFreeShaderFx(pToDelete->ptr);
		}
	}
	asResource_ClearDeletionQueue(shaderFxResourceType);
}

/*Everthing*/
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
	_ShaderFxManagerInit();
	asInitTexturePool();
	asInitSceneRenderer();
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
#endif
	asTexturePoolUpdate();
	asSceneRendererDraw(0);
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
#endif
	asShutdownSceneRenderer();
#if ASTRENGINE_NUKLEAR
	asShutdownGfxNk();
#endif
#if ASTRENGINE_DEARIMGUI
	asShutdownGfxImGui();
#endif
	asShutdownTexturePool();
	_ShaderFxGC();
#if ASTRENGINE_VK
	asVkFinalShutdown();
#endif
	SDL_DestroyWindow(asMainWindow);
}