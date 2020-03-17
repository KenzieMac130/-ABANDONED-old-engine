#ifndef _ASNUKLEARIMPLIMENTATION_H_
#define _ASNUKLEARIMPLIMENTATION_H_

#include "../common/asCommon.h"
#include "../renderer/asRenderFx.h"
#if ASTRENGINE_NUKLEAR
#ifdef __cplusplus
extern "C" {
#endif 

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
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
* @brief Reset nuklear for next frame
* @warning The engine update should handle this for you
*/
ASEXPORT void asNkReset();

/**
* @brief Push an event into nuklear
* @warning The engine update should handle this for you
* @warning this should be fed a pointer to an SDL_Event
*/
ASEXPORT void asNkPushEvent(void *pEvent);

/**
* @brief begin nuklear input (events)
* @warning The engine update should handle this for you
*/
ASEXPORT void asNkBeginInput();
/**
* @brief end nuklear input (events)
* @warning The engine update should handle this for you
*/
ASEXPORT void asNkEndInput();

/**
* @brief Shutdown the nuklear backend
* @warning The engine shutdown should handle this for you
*/
ASEXPORT void asShutdownNk();

/**
* @For internal use by shader system
*/
ASEXPORT asResults _asFillGfxPipeline_Nuklear(
	asBinReader* pShaderAsBin,
	asGfxAPIs api,
	asPipelineType pipelineType,
	void* pDesc,
	const char* pipelineName,
	void* pUserData);

#endif

#ifdef __cplusplus
}
#endif
#endif