#include "asEntry.h"
#include "../common/asCommon.h"
#include "asOsEvents.h"
#include "../renderer/asRendererCore.h"
#include "../resource/asUserFiles.h"
#if ASTRENGINE_NUKLEAR
#include "../nuklear/asNuklearImplimentation.h"
#endif
#if ASTRENGINE_DEARIMGUI
#include "../cimgui/asDearImGuiImplimentation.h"
#endif
#include <SDL.h>

bool gContinueLoop;
asLinearMemoryAllocator_t linearAllocator;

ASEXPORT int asIgnite(int argc, char *argv[], asAppInfo_t *pAppInfo, void *pCustomWindow)
{
	/*Info*/
	asDebugLog("%s %d.%d.%d", pAppInfo->pAppName, pAppInfo->appVersion.major, pAppInfo->appVersion.minor, pAppInfo->appVersion.patch);
	asDebugLog("astrengine %d.%d.%d", ASTRENGINE_VERSION_MAJOR, ASTRENGINE_VERSION_MINOR, ASTRENGINE_VERSION_PATCH);

	/*Memory*/
	asAllocInit_Linear(&linearAllocator, 100000);

	/*Init All Systems*/
	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER);
	asInitUserFiles(pAppInfo->pDevName, pAppInfo->pAppName);
	asInitResource();
	asInitGfx(&linearAllocator, pAppInfo, pCustomWindow);

#if ASTRENGINE_NUKLEAR
	asInitNk();
#endif
#if ASTRENGINE_DEARIMGUI
	asInitImGui();
#endif
	return 0; 
}

ASEXPORT void asShutdown(void)
{
#if ASTRENGINE_NUKLEAR
	asShutdownNk();
#endif
#if ASTRENGINE_DEARIMGUI
	asShutdownImGui();
#endif
	asShutdownGfx();
	asShutdownResource();
	asShutdownUserFiles();
	SDL_Quit();
	asAllocShutdown_Linear(&linearAllocator);
	asDebugLog("astrengine Quit...");
}

ASEXPORT int asLoopSingleShot(double time, asLoopDesc_t loopDesc)
{
	if (loopDesc.fpOnTick)
		loopDesc.fpOnTick(1.0 / 30);
	if (loopDesc.fpOnUpdate)
		loopDesc.fpOnUpdate(time);

	asImGuiEndFrame();
	asPollOSEvents();
#if ASTRENGINE_DEARIMGUI
	asImGuiPumpInput();
#endif
	asGfxRenderFrame();
	asImGuiNewFrame(time);
	return 0;
}

ASEXPORT int asEnterLoop(asLoopDesc_t loopDesc)
{
	gContinueLoop = true;
	float deltaTime = 1.0/1000;
	asTimer_t globalTimer = asTimerStart();
	while (gContinueLoop)
	{
		deltaTime = (float)asTimerSeconds(globalTimer, asTimerTicksElapsed(globalTimer));
		globalTimer = asTimerRestart(globalTimer);
		if (deltaTime < 0) { deltaTime = 0.000001f; }
		asLoopSingleShot(deltaTime, loopDesc);
	}
	return 0;
}

ASEXPORT void asExitLoop()
{
	gContinueLoop = false;
}