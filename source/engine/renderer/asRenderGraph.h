#ifndef _ASRENDERGRAPH_H_
#define _ASRENDERGRAPH_H_
#endif

#include "engine/common/asCommon.h"
#include "engine/resource/asResource.h"
#include "engine/common/reflection/asReflectIOBinary.h"
#include "engine/renderer/asRendererCore.h"
#include "engine/common/asBin.h"
#include "engine/renderer/asSceneRenderer.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct asRenderGraphAttachmentT* asRenderGraphAttachment;
typedef struct asRenderGraphBufferT* asRenderGraphBuffer;
typedef struct asRenderGraphStageT* asRenderGraphStage;
typedef struct asRenderGraphT* asRenderGraph;

typedef enum {
	AS_RENDERGRAPH_SCALE_ABSOLUTE,
	AS_RENDERGRAPH_SCALE_COMPOSITE_RELATIVE,
	AS_RENDERGRAPH_SCALE_PARENT_RELATIVE,
	AS_RENDERGRAPH_SCALE_COUNT,
	AS_RENDERGRAPH_SCALE_MAX = UINT32_MAX
} asRenderGraphAttachmentScaleType;

typedef enum {
	AS_RENDERGRAPH_STAGE_RAW_API,
	AS_RENDERGRAPH_STAGE_RASTER_FULLSCREEN_TRI,
	AS_RENDERGRAPH_STAGE_RASTER_PRIMITIVE_SUBMISSION,
	AS_RENDERGRAPH_STAGE_COUNT,
	AS_RENDERGRAPH_STAGE_MAX = UINT32_MAX
} asRenderGraphStageType;

typedef struct {
	const char* name;
	asColorFormat pixelFormat;
	asRenderGraphAttachmentScaleType scalingType;
	const char* scaleParentName;
	float scale[3];
	float mips;
	float layers;
	bool persistent;
} asRenderGraphAttachmentDesc;

typedef struct {
	const char* name;
	size_t size;
	asBufferUsageFlags usage;
	bool persistent;
} asRenderGraphBufferDesc;

typedef struct {
	const char* name;
	asRenderGraphStageType type;

	/*Clearing*/
	bool clearDepthChannel;
	float clearDepthValue;

	bool clearColorChannels[8];
	float clearColorValues[8][4];

	/*Primitive Type*/
	asPrimitiveSubmissionQueue primitiveQueue;

	/*Fullscreen Tri*/
	asShaderFx* pShaderFx;
} asRenderGraphStageDesc;

/**
* Entirety of a render graph
*/
typedef struct {
	const char* debugName;
	int viewportIdx;
} asRenderGraphDesc;

#ifdef __cplusplus
}
#endif