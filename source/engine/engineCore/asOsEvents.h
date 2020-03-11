#ifndef _ASOSEVENTS_H_
#define _ASOSEVENTS_H_

#include "../common/asCommon.h"
#ifdef __cplusplus
extern "C" {
#endif 
/**
* @file
* @brief Handles operating system events
*/

/**
* @brief Polls the operating system for events and populates subsystem command buffers
* @warning Don't touch this the engine loop should handle this for you
*/
ASEXPORT void asPollOSEvents();

#ifdef __cplusplus
}
#endif
#endif