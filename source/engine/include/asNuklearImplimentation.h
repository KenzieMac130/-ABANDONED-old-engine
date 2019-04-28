#pragma once

#include "asCommon.h"

#if ASTRENGINE_NUKLEAR
#include "asRendererCore.h"

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_UINT_DRAW_INDEX
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_FONT

/**
* @file
* @brief Implimentation of the Nuklear Immediate GUI library 
*/

/**
* @brief Return the internal main nuklear context
*/
ASEXPORT struct nk_context* asGetNuklearContextPtr();

/**
* @brief Initializes the nuklear backend
* @warning The engine ignite should handle this for you
*/
ASEXPORT void asInitNk();
/**
* @brief Draw the nuklear user interface
* @warning The engine update should handle this for you
*/
ASEXPORT void asNkDraw();

/**
* @brief Push an event into nuklear
* @warning The engine update should handle this for you
* @warning this should be fed a pointer to an SDL_Event
*/
ASEXPORT void asNkPushEvent(void *pEvent);

/**
* @brief Shutdown the nuklear backend
* @warning The engine shutdown should handle this for you
*/
ASEXPORT void asShutdownNk();
#endif