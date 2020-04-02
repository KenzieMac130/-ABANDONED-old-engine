#ifndef _ASENTRY_H_
#define _ASENTRY_H_

#include "../common/asCommon.h"
#ifdef __cplusplus
extern "C" {
#endif 

/**
* @file
* @brief Entry point functionality
*/

/**
* @brief Starts up astrengine
* @param argc pass your executable's arguement count
* @param argv pass your executable's arguement list
* @param pAppInfo a configuration for your app or game
* @param pCustomWindow existing OS specific window handle. (useful for integrating into editors)
* Most often you will want to pass null to create a new window or something like (void*)myHWND 
*/
ASEXPORT int asIgnite(int argc, char *argv[], asAppInfo_t *pAppInfo, void *pCustomWindow);
/**
* @brief A type for an update function that passes time as an input
*/
typedef void (*asUpdateFunction_t)(double);
/**
* @brief Description for entering a loop (callbacks)
*/
typedef struct
{
	asUpdateFunction_t fpOnUpdate;
	asUpdateFunction_t fpOnTick;
} asLoopDesc_t;
/**
* @brief Enters the main engine loop
*/
ASEXPORT int asEnterLoop(asLoopDesc_t loopDesc);
/**
* @brief Break out of the main engine loop
*/
ASEXPORT void asExitLoop();
/**
* @brief Single shot the engine loop (useful for integrating into editors)
*/
ASEXPORT int asLoopSingleShot(double time, asLoopDesc_t loopDesc);
/**
* @brief Shutdown the engine and release all resources
* @warning do not call this manually in the game loop! try hooking it up to atexit()
*/
ASEXPORT void asShutdown(void);

ASEXPORT void asToggleDevConsole();

#ifdef __cplusplus
}
#endif
#endif