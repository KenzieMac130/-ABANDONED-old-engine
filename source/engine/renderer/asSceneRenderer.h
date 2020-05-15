#ifndef _ASSCENERENDERER_H_
#define _ASSCENERENDERER_H_

#include "../engine/common/asCommon.h"
#include "../engine/renderer/asRendererCore.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t viewportIdx; /**< Viewport index (future use)*/
	uint32_t maxTransforms; /**< Maximum number of uploaded transforms*/
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

ASEXPORT asPrimitiveSubmissionQueue asSceneRendererCreateSubmissionQueue(asPrimitiveSubmissionQueueDesc* pDesc);
ASEXPORT asResults asSceneRendererDestroySubmissionQueue(asPrimitiveSubmissionQueue queue);

ASEXPORT asResults asSceneRendererSubmissionQueuePopulateBegin(asPrimitiveSubmissionQueue queue);
ASEXPORT asResults asSceneRendererSubmissionQueuePopulateEnd(asPrimitiveSubmissionQueue queue);
ASEXPORT asResults asSceneRendererSubmissionQueuePrepareFrameSubmit(asPrimitiveSubmissionQueue queue);

typedef int32_t asGfxDrawFlags;
typedef enum {
	AS_GFX_DRAW_FLAG_UINT32_INDICES = 1 << 0, /**< Uses uint32_t for indices instead of uint16_t*/
	AS_GFX_DRAW_FLAG_HW_SKINNED = 1 << 1, /**< Use Hardware Skinning to Offset into Transforms by Bone Index*/
} asGfxInstanceFlagBits;

typedef enum {
	AS_SCENE_RENDERPASS_SHADOW = 1 << 0,
	AS_SCENE_RENDERPASS_DEPTH_PREPASS = 1 << 1,
	AS_SCENE_RENDERPASS_GBUFFER_PASS = 1 << 2,
	AS_SCENE_RENDERPASS_DEFERRED_LIGHT = 1 << 3,
	AS_SCENE_RENDERPASS_WATER_DECAL = 1 << 4,
	AS_SCENE_RENDERPASS_WATER_SURFACE = 1 << 5,
	AS_SCENE_RENDERPASS_PARTICLE_SOFT = 1 << 6,
	AS_SCENE_RENDERPASS_GUI = 1 << 7,
	AS_SCENE_RENDERPASS_COUNT = 8,
	AS_SCENE_RENDERPASS_SOLID = AS_SCENE_RENDERPASS_SHADOW | AS_SCENE_RENDERPASS_DEPTH_PREPASS | AS_SCENE_RENDERPASS_GBUFFER_PASS,
	AS_SCENE_RENDERPASS_MAX = UINT32_MAX
} asSceneRenderPass;

/*Layout of structure is to ensure padding with vec4*/
typedef struct {
	float position[3]; /**< Position (XYZ)*/
	float relativeTime; /**< Relative Time Value*/
	float rotation[4]; /**< Rotation (XYZW)*/
	float scale[3]; /**< Scale (XYZ)*/
	float opacity; /**< Per-Transform Fade Opacity (Useful for LOD)*/
	float customProps[4]; /**< Custom Shader Props*/
} asGfxInstanceTransform;

typedef struct {
	float sortDistance; /**< Sort Distance (will be reversed on transparent renderpasses)*/
	asGfxDrawFlags flags; /**< Rendering Flags*/
	asSceneRenderPass renderPass; /**< "Renderpass" to target*/
	
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

ASEXPORT asResults asSceneRendererSubmissionAddBoundingBoxes(asPrimitiveSubmissionQueue queue, uint32_t boundsCount, float* pBoundingBoxes);
ASEXPORT asResults asSceneRendererSubmissionAddTransforms(asPrimitiveSubmissionQueue queue, uint32_t transformCount, asGfxInstanceTransform* pTransforms, uint32_t* pBeginningTransformOffset);
ASEXPORT asResults asSceneRendererSubmissionAddPrimitiveGroups(asPrimitiveSubmissionQueue queue, uint32_t primitiveCount, asGfxPrimativeGroupDesc* pDescs);

typedef struct {
	int32_t viewportIdx;
	int32_t subViewportIdx;
	int32_t flags;
	float time;
	vec3 viewPos;
	quat viewRotation;
	float fov;
	float clipStart;
	float clipEnd;
	int32_t debugState;
} asGfxViewerParamsDesc;

ASEXPORT asResults asSceneRendererSetViewerParams(size_t descCount, asGfxViewerParamsDesc* pDescs);

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