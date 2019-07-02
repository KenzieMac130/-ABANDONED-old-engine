#include "include/asEntry.h"
#include "include/asCommon.h"
#include "include/asOsEvents.h"
#include "include/asRendererCore.h"
#if ASTRENGINE_NUKLEAR
#include "include/asNuklearImplimentation.h"
#endif
#include <SDL.h>

bool gContinueLoop;

ASEXPORT void asShutdown(void)
{
#if ASTRENGINE_NUKLEAR
	asShutdownNk();
#endif
	asShutdownGfx();
	asShutdownResource();
	SDL_Quit();
	asAllocShutdown_Linear();
	asDebugLog("astrengine Quit...");
}

ASEXPORT int asIgnite(int argc, char *argv[], asAppInfo_t *pAppInfo, void *pCustomWindow)
{
	/*Defaults*/
	if (!pAppInfo->pAppName)
		pAppInfo->pAppName = "UNTITLED";
	if(!pAppInfo->pGfxIniName)
		pAppInfo->pGfxIniName = "graphics.ini";

	/*Info*/
	asDebugLog("%s %d.%d.%d\n", pAppInfo->pAppName, pAppInfo->appVersion.major, pAppInfo->appVersion.minor, pAppInfo->appVersion.patch);
	asDebugLog("astrengine %d.%d.%d\n", ASTRENGINE_VERSION_MAJOR, ASTRENGINE_VERSION_MINOR, ASTRENGINE_VERSION_PATCH);

	/*Memory*/
	asAllocInit_Linear(100000);

	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER);
	asInitResource("resourceManifest.ini");
	asInitGfx(pAppInfo, pCustomWindow);

#if ASTRENGINE_NUKLEAR
	asInitNk();
#endif
	return 0; 
}

ASEXPORT int asLoopSingleShot(double time, asLoopDesc_t loopDesc)
{
	if (loopDesc.fpOnTick)
		loopDesc.fpOnTick(1.0 / 30);
	if (loopDesc.fpOnUpdate)
		loopDesc.fpOnUpdate(time);
	asPollOSEvents();
	asGfxRenderFrame();
	return 0;
}

ASEXPORT int asEnterLoop(asLoopDesc_t loopDesc)
{
	gContinueLoop = true;
	double deltaTime = 1.0/60;
	while (gContinueLoop)
	{
		asLoopSingleShot(deltaTime, loopDesc);
	}
	return 0;
}

ASEXPORT void asExitLoop()
{
	gContinueLoop = false;
}