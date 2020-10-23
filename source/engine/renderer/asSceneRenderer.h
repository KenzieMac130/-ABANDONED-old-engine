#ifndef _ASSCENERENDERER_H_
#define _ASSCENERENDERER_H_

#include "engine/common/asCommon.h"
#include "engine/renderer/asRendererCore.h"
#include "engine/renderer/asRenderGraph.h"

#ifdef __cplusplus
extern "C" {
#endif

/*Viewer*/
typedef struct asGfxViewerT* asGfxViewer;
typedef struct {
	int32_t screenIdx;
	int32_t viewportIdx;
	int32_t flags;
	float time;
	vec3 viewPosition;
	quat viewRotation;
	bool ortho;
	float fov;
	float clipStart;
	float clipEnd;
	int32_t debugState;
} asGfxViewerParamsDesc;

ASEXPORT asResults asSceneRendererCreateViewer(asGfxViewer* pViewer);
ASEXPORT asResults asSceneRendererSetViewerParams(asGfxViewer viewer, size_t descCount, asGfxViewerParamsDesc* pDescs);
ASEXPORT void asSceneRendererDestroyViewer(asGfxViewer viewer);

/*Transform*/
typedef struct asSceneRendererTransformPoolT* asSceneRendererTransformPool;

/*Layout of structure is to ensure padding with vec4*/
typedef struct {
	float position[3]; /**< Position (XYZ)*/
	float relativeTime; /**< Relative Time Value (Useful for Anim)*/
	float rotation[4]; /**< Rotation (XYZW)*/
	float scale[3]; /**< Scale (XYZ)*/
	float opacity; /**< Per-Transform Fade Opacity (Useful for LOD)*/
	float boundBoxMin[3]; /**< Bounding Box Minimum (XYZ)*/
	float bsphere; /**< Bounding sphere*/
	float boundBoxMax[3]; /**< Bounding Box Maximum (XYZ)*/
	float random; /**< Per-Transform Seed Value (Useful for Variance)*/
	float customProps[4]; /**< Custom Shader Props*/
} asGfxInstanceTransform;

ASEXPORT asResults asSceneRendererTransformPoolCreate(asSceneRendererTransformPool* pPool, uint32_t maxTransforms);
ASEXPORT asResults asSceneRendererTransformPoolAddTransforms(asSceneRendererTransformPool pool, uint32_t transformCount, asGfxInstanceTransform* pTransforms, uint32_t* pBeginningTransformOffset);
ASEXPORT asResults asSceneRendererTransformPoolPopulateBegin(asSceneRendererTransformPool pool);
ASEXPORT asResults asSceneRendererTransformPoolPopulateEnd(asSceneRendererTransformPool pool);
ASEXPORT asResults asSceneRendererTransformPoolDestroy(asSceneRendererTransformPool pool);

/*Primitive Submission*/
typedef struct {
	asRenderGraphStage graphStage; /**< Render graph stage*/
	asGfxViewer viewer; /**< View parameters*/
	asSceneRendererTransformPool transformPool; /**< Pool of transformations*/

	uint32_t maxPrimitives; /**< Maximum number of uploaded primatives*/
	uint32_t maxInstances; /**< Maximum number of instances (post batching)*/
	bool disableInstanceSort; /**< Disable sorting of instances*/
	bool disableInstanceMerge; /**< Disable merging of instances*/
} asPrimitiveSubmissionQueueDesc;
/**
* Primitive Submission Queue 
* (Can be recorded once or treated like an Immediate Buffer)
* Function calls to this should be thread isolated, creation and destruction is not threadsafe
*/
typedef struct asPrimitiveSubmissionQueueT* asPrimitiveSubmissionQueue;

ASEXPORT asResults asInitSceneRenderer();
ASEXPORT asResults asTriggerResizeSceneRenderer();
ASEXPORT asResults asShutdownSceneRenderer();

ASEXPORT asResults asSceneRendererSubmissionQueueCreate(asPrimitiveSubmissionQueue* pQueue, asPrimitiveSubmissionQueueDesc* pDesc);
ASEXPORT asResults asSceneRendererSubmissionQueueDestroy(asPrimitiveSubmissionQueue queue);

ASEXPORT asResults asSceneRendererSubmissionQueuePopulateBegin(asPrimitiveSubmissionQueue queue);
ASEXPORT asResults asSceneRendererSubmissionQueuePopulateEnd(asPrimitiveSubmissionQueue queue);
ASEXPORT asResults asSceneRendererSubmissionQueuePrepareFrameSubmit(asPrimitiveSubmissionQueue queue);

typedef int32_t asGfxDrawFlags;
typedef enum {
	AS_GFX_DRAW_FLAG_UINT32_INDICES = 1 << 0, /**< Uses uint32_t for indices instead of uint16_t*/
	AS_GFX_DRAW_FLAG_HW_SKINNED = 1 << 1, /**< Use Hardware Skinning to Offset into Transforms by Bone Index*/
} asGfxInstanceFlagBits;

typedef struct {
	float sortDistance; /**< Sort Distance (will be reversed on transparent renderpasses)*/
	asGfxDrawFlags flags; /**< Rendering Flags*/
	
	uint32_t vertexByteOffset; /**< Vertex buffer offset in bytes*/
	uint32_t vertexStart; /**< Beginning Vertex*/
	uint32_t vertexCount; /**< Vertex count (only used if index buffer is unspecified)*/
	asBufferHandle_t vertexBuffer; /**< Vertex Buffer (optional)*/
	
	uint32_t indexByteOffset; /**< Index offset*/
	uint32_t indexStart; /**< Beginning Index*/
	uint32_t indexCount; /**< Index count*/
	asBufferHandle_t indexBuffer; /**< Index Buffer (optional)*/
	
	asShaderFx* pShaderFx; /**< Shader FX to Apply*/
	int32_t materialId; /**< Material index to Bind*/
	
	uint32_t baseInstanceCount; /**< Number of instanced (expected to be transformCount unless skinned)*/
	
	uint16_t transformOffset; /**< Offset in transforms (expected to be retrieved by last pBeginningTransformOffset)*/
	uint16_t transformOffsetPreviousFrame; /**< Similar as above but additional transforms for velocity (make same as current frame if no movement)*/
	uint16_t transformCount; /**< Number of transforms (expected to be same as instance count unless skinned)*/
	
	uint32_t stencilWriteBits; /**< Base stencil bits*/

	int32_t debugState; /*Debug State Flag*/
	
	void* pCustomRenderData; /**< Custom Data (reserved)*/
} asGfxPrimativeGroupDesc;

ASEXPORT asResults asSceneRendererSubmissionAddPrimitiveGroups(asPrimitiveSubmissionQueue queue, uint32_t primitiveCount, asGfxPrimativeGroupDesc* pDescs);

ASEXPORT asResults asSceneRendererUploadDebugDraws(int32_t viewport);
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