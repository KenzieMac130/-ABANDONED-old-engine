#ifndef _ASSCENERENDERER_H_
#define _ASSCENERENDERER_H_

#include "../engine/common/asCommon.h"
#include "../engine/renderer/asRendererCore.h"
#ifdef __cplusplus
extern "C" {
#endif 

ASEXPORT asResults asInitSceneRenderer();
ASEXPORT asResults asShutdownSceneRenderer();

ASEXPORT asResults asSceneRendererDraw(int32_t viewport);

/**
* @For internal use by shader system
*/
ASEXPORT asResults _asFillGfxPipeline_Scene(
	asBinReader* pShaderAsBin,
	asGfxAPIs api,
	asPipelineType pipelineType,
	void* pDesc,
	const char* pipelineName,
	asPipelineHandle* pPipelineOut,
	void* pUserData);

#ifdef __cplusplus
}
#endif
#endif