#include "asEntry.h"
#include "../common/asCommon.h"
#include "asOsEvents.h"
#include "../renderer/asRendererCore.h"
#include "../resource/asUserFiles.h"
#include "../common/preferences/asPreferences.h"
#if ASTRENGINE_NUKLEAR
#include "../nuklear/asNuklearImplimentation.h"
#endif
#if ASTRENGINE_DEARIMGUI
#include "../cimgui/asDearImGuiImplimentation.h"
#endif
#include <SDL.h>

#include "../guiTools/cmdConsole/asCmdConsole.h"

int32_t gContinueLoop;
int32_t gDevConsoleToggleable = 1;
#ifdef NDEBUG
bool gShowDevConsole = false;
#else
bool gShowDevConsole = true;
#endif

#define GLOBAL_INI_NAME "astrengineConfig.ini"

asResults _commandQuit(const char* propName, void* pCurrentValue, void* pNewValueTmp, void* pUserData)
{
	gContinueLoop = false;
	return AS_SUCCESS;
}

ASEXPORT int asIgnite(int argc, char *argv[], asAppInfo_t *pAppInfo, void *pCustomWindow)
{
	/*Info*/
	asDebugLog("%s %d.%d.%d", pAppInfo->pAppName, pAppInfo->appVersion.major, pAppInfo->appVersion.minor, pAppInfo->appVersion.patch);
	asDebugLog("astrengine %d.%d.%d", ASTRENGINE_VERSION_MAJOR, ASTRENGINE_VERSION_MINOR, ASTRENGINE_VERSION_PATCH);

	/*Init All Systems*/
	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER);
	asInitUserFiles(pAppInfo->pDevName, pAppInfo->pAppName);

	/*Set Debug Log Output File*/
	char debugLogPath[4096];
	memset(debugLogPath, 0, 4096);
	asUserFileMakePath("astrengineLog.txt", debugLogPath, 4096);
	_asDebugLoggerInitializeFile(debugLogPath);

	/*Init Preferences*/
	asPreferenceManager* pPrefMan;
	asPreferenceManagerCreate(&pPrefMan);
	_asSetGlobalPrefs(pPrefMan);
	asPreferencesLoadIni(pPrefMan, GLOBAL_INI_NAME);

	/*Set Global Pref Data*/
	asPreferencesRegisterOpenSection(asGetGlobalPrefs(), "core");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "devConsoleEnabled", &gShowDevConsole, 0, 1, false, NULL, NULL, "Show Developer Console");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "devConsoleToggleable", &gDevConsoleToggleable, 0, 1, true, NULL, NULL, "Dev Console Toggleable Developer Console");
	asPreferencesRegisterNullFunction(asGetGlobalPrefs(), "quit", _commandQuit, false, NULL, NULL, "Quit Engine (Alt+F4)");
	asPreferencesLoadSection(asGetGlobalPrefs(), "core");

	asGuiToolCommandConsole_RegisterPrefManager(pPrefMan, "as");

	asInitResource();

	asInitGfx(pAppInfo, pCustomWindow);

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
	
	asPreferencesSaveSectionsToIni(asGetGlobalPrefs(), GLOBAL_INI_NAME);
	asPreferenceManagerDestroy(asGetGlobalPrefs());
	asShutdownUserFiles();
	SDL_Quit();
	asDebugLog("astrengine Quit...");
}

ASEXPORT void asToggleDevConsole()
{
	if (!gDevConsoleToggleable) { return; }

	if (gShowDevConsole) { gShowDevConsole = false; }
	else { gShowDevConsole = true; }
}

ASEXPORT int asLoopSingleShot(double time, asLoopDesc_t loopDesc)
{
	/*Dev Console*/
	if(gShowDevConsole)
		asGuiToolCommandConsoleUI();

	/*Update Callbacks*/
	if (loopDesc.fpOnTick)
		loopDesc.fpOnTick(1.0 / 30);
	if (loopDesc.fpOnUpdate)
		loopDesc.fpOnUpdate(time);

	asGfxInternalDebugDraws();
	asImGuiEndFrame();
	asPollOSEvents();
#if ASTRENGINE_DEARIMGUI
	asImGuiPumpInput();
#endif
	asGfxRenderFrame();
#if ASTRENGINE_DEARIMGUI
	asImGuiNewFrame(time);
#endif
	return 0;
}

ASEXPORT int asEnterLoop(asLoopDesc_t loopDesc)
{
	float deltaTime = 1.0/30; /*Initial Delta Time*/
	asTimer_t globalTimer = asTimerStart();
	gContinueLoop = true;
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