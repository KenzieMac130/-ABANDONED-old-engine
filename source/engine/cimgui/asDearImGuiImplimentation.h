#ifndef _ASDEARIMGUIIMPLIMENTATION_H_
#define _ASDEARIMGUIIMPLIMENTATION_H_

#include "../common/asCommon.h"
#include "../renderer/asRenderFx.h"
#if ASTRENGINE_DEARIMGUI
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "../thirdparty/cimgui/cimgui.h"
#ifdef __cplusplus
extern "C" {
#endif 

/**
* @file
* @brief Implimentation of the DearImgui Immediate GUI library via CImgui
*/

/**
* @brief Initializes the DearImgui backend
* @warning The engine ignite should handle this for you
*/
ASEXPORT void asInitImGui();
/**
* @brief Draw the DearImgui user interface
* @warning The engine update should handle this for you
*/
ASEXPORT void asImGuiDraw(int32_t viewport);

/**
* @brief Reset DearImgui for next frame
* @warning The engine update should handle this for you
*/
ASEXPORT void asImGuiNewFrame(float time);

/**
* @brief Reset DearImgui for next frame
* @warning The engine update should handle this for you
*/
ASEXPORT void asImGuiEndFrame();

/**
* @brief Reset DearImgui for next frame
* @warning The engine update should handle this for you
*/
ASEXPORT void asImGuiReset();

/**
* @brief Push an event into DearImgui
* @warning The engine update should handle this for you
* @warning this should be fed a pointer to an SDL_Event
*/
ASEXPORT void asImGuiPushEvent(void *pEvent);

/**
* @brief begin DearImgui input (events)
* @warning The engine update should handle this for you
*/
ASEXPORT void asImGuiPumpInput();

/**
* @brief Shutdown the DearImgui backend
* @warning The engine shutdown should handle this for you
*/
ASEXPORT void asShutdownImGui();
/**
* @brief Shutdown the DearImgui backend's graphics resources
* @warning The engine shutdown should handle this for you
*/
ASEXPORT void asShutdownGfxImGui();

/**
* @For internal use by shader system
*/
ASEXPORT asResults _asFillGfxPipeline_DearImgui(
	asBinReader* pShaderAsBin,
	asGfxAPIs api,
	asPipelineType pipelineType,
	void* pDesc,
	const char* pipelineName,
	asPipelineHandle* pPipelineOut,
	void* pUserData);

#endif

#ifdef __cplusplus
}
#endif
#endif