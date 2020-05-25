#include "asSceneRenderer.h"

#include "asBindlessTexturePool.h"
#include "cglm/box.h"

#if ASTRENGINE_VK
#include "../renderer/vulkan/asVulkanBackend.h"
#include <SDL_vulkan.h>

#include "../model/runtime/asModelRuntime.h"

VkPipelineLayout scenePipelineLayout;
VkRenderPass sceneRenderPass;
VkFramebuffer sceneFramebuffer;

VkDescriptorSetLayout sceneViewDescSetLayout;
VkDescriptorPool sceneDescriptorPool;
#define SCENE_MAX_DESC_SETS 16

#define AS_BINDING_VIEWER_UBO 0
#define AS_DESCSET_VIEWER_UBO 1
#endif

/*Primitive Submission Queue*/
struct primInstanceData {
	uint16_t previousFrameTransform;
	uint16_t currentFrameTransform;
};

struct asPrimitiveSubmissionQueueT {
	enum PrimitiveSubmissionQueueState state;
	int32_t currentFrame;

	asBufferHandle_t offsetBuffs[AS_MAX_INFLIGHT];
	asBufferHandle_t transformBuffs[AS_MAX_INFLIGHT];

	uint32_t primitiveGroupMax;
	uint32_t initialPrimitiveGroupCount;
	asGfxPrimativeGroupDesc* pInitialPrimGroups;

	uint32_t transformCount;
	uint32_t transformMax;
	asGfxInstanceTransform* pTransforms;
	void* _transformBufferMappings[AS_MAX_INFLIGHT];

	uint32_t instanceMax;
	uint32_t instanceCount;
	struct primInstanceData* pInstanceTransformOffsets;
	void* _InstanceBufferMappings[AS_MAX_INFLIGHT];

	vec3 boundingBox[2];

#if ASTRENGINE_VK
	VkCommandPool vCommandPool;
	VkCommandBuffer vSecondaryCommandBuffers[AS_MAX_INFLIGHT][AS_SCENE_RENDERPASS_COUNT];
#endif
	int32_t renderpassPopulatedFlags;
};

/*Scene Viewport*/
#define AS_MAX_SUBVIEWPORTS 6
struct sceneUbo {
	mat4 viewMatrix[AS_MAX_SUBVIEWPORTS];
	mat4 projMatrix[AS_MAX_SUBVIEWPORTS];
	vec4 viewPosition;
	vec4 viewRotation;
	float width;
	float height;
	float time;
	int32_t debug;
};

void sceneUbo_SetMatrices(struct sceneUbo* pUbo,
	int subViewportIdx,
	vec3 viewPos,
	quat viewRot,
	float near,
	float far,
	float fov,
	bool ortho,
	vec2 renderDim)
{
	/*Set Render Dimensions*/
	pUbo->width = renderDim[0];
	pUbo->height = renderDim[1];

	/*Create View Matrix*/
	glm_mat4_identity(pUbo->viewMatrix[subViewportIdx]);
	glm_translate(pUbo->viewMatrix[subViewportIdx], viewPos);
	glm_quat_rotate(pUbo->viewMatrix[subViewportIdx], viewRot, pUbo->viewMatrix[subViewportIdx]);
	glm_mat4_inv(pUbo->viewMatrix[subViewportIdx], pUbo->viewMatrix[subViewportIdx]);

	/*Fill out Camera Setup*/
	glm_vec4_zero(pUbo->viewPosition);
	glm_vec3_copy(viewPos, pUbo->viewPosition);
	glm_vec4_copy(viewRot, pUbo->viewRotation);

	/*Create Projection Matrix*/
	const float aspect = renderDim[0] / renderDim[1];
	if (ortho) {
		const float size = fov;
		glm_ortho(-size * aspect,
			size * aspect,
			-size,
			size,
			far,
			near,
			pUbo->projMatrix[subViewportIdx]);
	}
	else {
		glm_perspective_infinite(glm_rad(fov), aspect, near, pUbo->projMatrix[subViewportIdx]);
	}
#ifdef ASTRENGINE_VK
	pUbo->projMatrix[subViewportIdx][1][1] *= -1.0f; /*Vulkan NDC*/
#endif
}

struct sceneViewport
{
	struct asPrimitiveSubmissionQueueT** pPrimQueues;
	struct sceneUbo* sceneUboData[AS_MAX_INFLIGHT];
	asBufferHandle_t sceneUboBuffer[AS_MAX_INFLIGHT];
#ifdef ASTRENGINE_VK
	VkDescriptorSet vDescriptorSets[AS_MAX_INFLIGHT];
#endif
};
struct sceneViewport mainSceneViewport;

void sceneViewport_Init(struct sceneViewport* pViewport)
{
	memset(pViewport, 0, sizeof(*pViewport));

	/*Uniform Buffers*/
	asBufferDesc_t buffDesc = asBufferDesc_Init();
	buffDesc.bufferSize = sizeof(struct sceneUbo);
	buffDesc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
	buffDesc.usageFlags = AS_BUFFERUSAGE_UNIFORM;
	buffDesc.initialContentsBufferSize = buffDesc.bufferSize;
	buffDesc.pInitialContentsBuffer = &(struct sceneUbo) { 2 };
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		pViewport->sceneUboBuffer[i] = asCreateBuffer(&buffDesc);
#if ASTRENGINE_VK
		asVkAllocation_t alloc = asVkGetAllocFromBuffer(pViewport->sceneUboBuffer[i]);
		asVkMapMemory(alloc, 0, alloc.size, &pViewport->sceneUboData[i]);
#endif
	}
	/*Descriptor Set*/
	{
		VkDescriptorSetLayout layouts[AS_MAX_INFLIGHT];
		for (int i = 0; i < AS_MAX_INFLIGHT; i++) { layouts[i] = sceneViewDescSetLayout; }
		VkDescriptorSetAllocateInfo descSetAllocInfo = (VkDescriptorSetAllocateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descSetAllocInfo.descriptorPool = sceneDescriptorPool;
		descSetAllocInfo.descriptorSetCount = AS_MAX_INFLIGHT;
		descSetAllocInfo.pSetLayouts = layouts;
		if (vkAllocateDescriptorSets(asVkDevice, &descSetAllocInfo, &pViewport->vDescriptorSets) != VK_SUCCESS)
			asFatalError("vkAllocateDescriptorSets() Failed to allocate pViewport->vDescriptorSets");

		for (int i = 0; i < AS_MAX_INFLIGHT; i++)
		{
			VkWriteDescriptorSet descSetWrite = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descSetWrite.dstSet = pViewport->vDescriptorSets[i];
			descSetWrite.dstBinding = AS_BINDING_VIEWER_UBO;
			descSetWrite.descriptorCount = 1;
			descSetWrite.dstArrayElement = 0;
			descSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descSetWrite.pBufferInfo = &(VkDescriptorBufferInfo) {
				.buffer = asVkGetBufferFromBuffer(pViewport->sceneUboBuffer[i]),
				.offset = 0,
				.range = sizeof(struct sceneUbo)
			};
			vkUpdateDescriptorSets(asVkDevice, 1, &descSetWrite, 0, NULL);
		}
	}
}

void sceneViewport_FlushGPU(struct sceneViewport* pViewport)
{
#if ASTRENGINE_VK
	asVkAllocation_t bufferData = asVkGetAllocFromBuffer(pViewport->sceneUboBuffer[asVkCurrentFrame]);
	asVkFlushMemory(bufferData);
#endif
}

void sceneViewport_Destroy(struct sceneViewport* pViewport)
{
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
#if ASTRENGINE_VK
		asVkUnmapMemory(asVkGetAllocFromBuffer(pViewport->sceneUboBuffer[i]));
#endif
		asReleaseBuffer(pViewport->sceneUboBuffer[i]);
	}
}

struct scenePushConstants
{
	int32_t materialIdx;
	int32_t transformOffsetCurrent;
	int32_t transformOffsetLast;
	int32_t debugIdx;
};

/*Viewer Params*/
ASEXPORT asResults asSceneRendererSetViewerParams(size_t descCount, asGfxViewerParamsDesc* pDescs)
{
	ASASSERT(descCount);
	ASASSERT(pDescs);
#if ASTRENGINE_VK
const int currentUboIdx = asVkCurrentFrame;
#endif
	struct sceneUbo* pUboData = mainSceneViewport.sceneUboData[currentUboIdx];
	const asGfxViewerParamsDesc desc = pDescs[0];

	int32_t w, h;
	asGetRenderDimensions(desc.viewportIdx, true, &w, &h);
	sceneUbo_SetMatrices(pUboData,
		desc.subViewportIdx,
		desc.viewPosition,
		desc.viewRotation,
		desc.clipStart, desc.clipEnd,
		desc.fov,
		false,
		(vec2) {(float)w, (float)h});

	pUboData->time = desc.time;
	pUboData->debug = desc.debugState;
	return AS_SUCCESS;
}

/*Record Command Buffer*/
void recordSecondaryCommands(uint32_t primCount,
	asGfxPrimativeGroupDesc* pPrims,
	asSceneRenderPass scenePass,
	struct sceneViewport* pViewport,
	float viewport[4],
#if ASTRENGINE_VK
	VkCommandBuffer vCmd,
#else
	void* _UNK,
#endif
	int bufferedFrame
){
#if ASTRENGINE_VK
	VkCommandBufferInheritanceInfo inheritInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	//Todo: Renderpass selection
	inheritInfo.renderPass = sceneRenderPass;
	inheritInfo.subpass = 0;
	inheritInfo.framebuffer = sceneFramebuffer;

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = &inheritInfo;
	vkBeginCommandBuffer(vCmd, &beginInfo);

	/*Viewport*/
	VkViewport vkViewport = (VkViewport){ 0 };
	vkViewport.minDepth = 0.0f;
	vkViewport.maxDepth = 1.0f;
	vkViewport.x = (float)viewport[2];
	vkViewport.y = (float)viewport[3];
	vkViewport.width = (float)viewport[0];
	vkViewport.height = (float)viewport[1];
	vkCmdSetViewport(vCmd, 0, 1, &vkViewport);

	/*Scissor*/
	VkRect2D scissor;
	scissor.offset.x = (int32_t)viewport[2];
	scissor.offset.y = (int32_t)viewport[3];
	scissor.extent.width = (uint32_t)viewport[0];
	scissor.extent.height = (uint32_t)viewport[1];
	vkCmdSetScissor(vCmd, 0, 1, &scissor);

	/*Bind Descriptor Sets*/
	asTexturePoolBindCmd(AS_GFXAPI_VULKAN, &vCmd, &scenePipelineLayout, bufferedFrame);
	vkCmdBindDescriptorSets(vCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		scenePipelineLayout, AS_DESCSET_VIEWER_UBO, 1, &pViewport->vDescriptorSets[bufferedFrame], 0, NULL);

	VkPipeline lastPipeline = VK_NULL_HANDLE;
	uint32_t lastStencilWrite = 0;
	bool firstLoop = true;
	for (uint32_t i = 0; i < primCount; i++)
	{
		const asGfxPrimativeGroupDesc prim = pPrims[i];
		const uint32_t instanceCount = prim.baseInstanceCount * 1;
		const uint32_t instanceStart = 0;

		/*Continue if not a part of the pass*/
		if (!(prim.renderPass & scenePass)) { continue; }

		/*Bind Pipeline*/
		ASASSERT(prim.pShaderFx);
		VkPipeline nextPipeline = (VkPipeline)prim.pShaderFx->pipelines[0];
		if (nextPipeline != lastPipeline)
		{
			vkCmdBindPipeline(vCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, nextPipeline);
			lastPipeline = nextPipeline;
		}

		/*Push Material Offset*/
		struct scenePushConstants pushConstants = { 0 };
		pushConstants.transformOffsetLast = prim.transformOffsetPreviousFrame;
		pushConstants.transformOffsetCurrent = prim.transformOffset;
		pushConstants.debugIdx = prim.debugState;
		pushConstants.materialIdx = prim.materialId;
		vkCmdPushConstants(vCmd, scenePipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(struct scenePushConstants), &pushConstants);

		/*Bind Vertex Buffer*/
		if (asHandleValid(prim.vertexBuffer))
		{
			VkBuffer vBuff = asVkGetBufferFromBuffer(prim.vertexBuffer);
			VkDeviceSize byteOffset = prim.vertexByteOffset;
			vkCmdBindVertexBuffers(vCmd, 0, 1, &vBuff, &byteOffset);
		}

		/*Stencil Write*/
		if (lastStencilWrite != prim.stencilWriteBits || firstLoop)
		{
			vkCmdSetStencilWriteMask(vCmd, VK_STENCIL_FRONT_AND_BACK, prim.stencilWriteBits);
		}

		/*Indexed Draw*/
		if (asHandleValid(prim.indexBuffer))
		{
			/*Bind Index Buffer*/
			VkBuffer iBuff = asVkGetBufferFromBuffer(prim.indexBuffer);
			VkIndexType iType = prim.flags & AS_GFX_DRAW_FLAG_UINT32_INDICES ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
			vkCmdBindIndexBuffer(vCmd, iBuff, prim.indexByteOffset, iType);

			vkCmdDrawIndexed(vCmd, prim.indexCount, instanceCount, prim.indexStart, prim.vertexStart, instanceStart);
		}
		else /*Non-indexed Draw*/
		{
			vkCmdDraw(vCmd, prim.vertexCount, instanceCount, prim.vertexStart, instanceStart);
		}

		firstLoop = false;
	}

	vkEndCommandBuffer(vCmd);
#endif
}

void _registerSubmissionQueue(struct asPrimitiveSubmissionQueueT* pQueue)
{
	arrput(mainSceneViewport.pPrimQueues, pQueue);
}

void _unregisterSubmissionQueue(struct asPrimitiveSubmissionQueueT* pQueue)
{
	for (int i = 0; i < arrlen(mainSceneViewport.pPrimQueues); i++) {
		if (mainSceneViewport.pPrimQueues[i] == pQueue) {
			arrdel(mainSceneViewport.pPrimQueues, i);
			return;
		}
	}
}

enum PrimitiveSubmissionQueueState {
	PRIMITIVE_SUBMISSION_QUEUE_STATE_EMPTY,
	PRIMITIVE_SUBMISSION_QUEUE_STATE_RECORDING,
	PRIMITIVE_SUBMISSION_QUEUE_STATE_RECORDED,
	PRIMITIVE_SUBMISSION_QUEUE_STATE_READY,
	PRIMITIVE_SUBMISSION_QUEUE_STATE_INVALID
};

ASEXPORT asPrimitiveSubmissionQueue asSceneRendererCreateSubmissionQueue(asPrimitiveSubmissionQueueDesc* pDesc)
{
	asPrimitiveSubmissionQueue queue = asMalloc(sizeof(struct asPrimitiveSubmissionQueueT));
	memset(queue, 0, sizeof(struct asPrimitiveSubmissionQueueT));
	queue->state = PRIMITIVE_SUBMISSION_QUEUE_STATE_EMPTY;
	queue->primitiveGroupMax = pDesc->maxPrimitives;
	queue->transformMax = pDesc->maxTransforms;
	queue->instanceMax = pDesc->maxInstances;
	ASASSERT(queue);

	asBufferDesc_t offsetBuffDesc = asBufferDesc_Init();
	offsetBuffDesc.usageFlags = AS_BUFFERUSAGE_STORAGE;
	offsetBuffDesc.pDebugLabel = "InstanceOffsetBuffer";
	offsetBuffDesc.bufferSize = queue->instanceMax * sizeof(struct primInstanceData);
	offsetBuffDesc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
	asBufferDesc_t transformBuffDesc = offsetBuffDesc;
	transformBuffDesc.bufferSize = queue->transformMax * sizeof(asGfxInstanceTransform);
	transformBuffDesc.pDebugLabel = "InstanceTransformBuffer";
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		queue->offsetBuffs[i] = asCreateBuffer(&offsetBuffDesc);
		queue->transformBuffs[i] = asCreateBuffer(&transformBuffDesc);
#if ASTRENGINE_VK
		asVkAllocation_t offsAlloc = asVkGetAllocFromBuffer(queue->offsetBuffs[i]);
		asVkMapMemory(offsAlloc, 0, offsAlloc.size, &queue->_InstanceBufferMappings[i]);
		asVkAllocation_t xformAlloc = asVkGetAllocFromBuffer(queue->transformBuffs[i]);
		asVkMapMemory(xformAlloc, 0, xformAlloc.size, &queue->_transformBufferMappings[i]);
#endif
	}
	const size_t allocSize = (size_t)queue->primitiveGroupMax * sizeof(asGfxPrimativeGroupDesc);
	queue->pInitialPrimGroups = asMalloc(allocSize);
	memset(queue->pInitialPrimGroups, 0, allocSize);

#if ASTRENGINE_VK
	/*Create Command Buffer Pool*/
	VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.queueFamilyIndex = asVkQueueFamilyIndices.graphicsIdx;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	if (vkCreateCommandPool(asVkDevice, &poolInfo, AS_VK_MEMCB, &queue->vCommandPool) != VK_SUCCESS) {
		asFatalError("vkCreateCommandPool() Failed to create queue->vCommandPool");
	}
#endif

	_registerSubmissionQueue(queue);
	return queue;
}

ASEXPORT asResults asSceneRendererDestroySubmissionQueue(asPrimitiveSubmissionQueue queue)
{
	_unregisterSubmissionQueue(queue);
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		if (!asHandleValid(queue->offsetBuffs[i])) { 
			continue;
		}
#if ASTRENGINE_VK
		asVkUnmapMemory(asVkGetAllocFromBuffer(queue->offsetBuffs[i]));
		asVkUnmapMemory(asVkGetAllocFromBuffer(queue->transformBuffs[i]));
#endif
		asReleaseBuffer(queue->offsetBuffs[i]);
		asReleaseBuffer(queue->transformBuffs[i]);
	}
	asFree(queue->pInitialPrimGroups);
#if ASTRENGINE_VK
	vkDestroyCommandPool(asVkDevice, queue->vCommandPool, AS_VK_MEMCB);
#endif
	asFree(queue);
	return AS_SUCCESS;
}

ASEXPORT asResults asSceneRendererSubmissionQueuePopulateBegin(asPrimitiveSubmissionQueue queue)
{
	queue->pInstanceTransformOffsets = queue->_InstanceBufferMappings[queue->currentFrame];
	queue->pTransforms = queue->_transformBufferMappings[queue->currentFrame];

	/*Reset Bounding Box*/
	glm_aabb_invalidate(queue->boundingBox);
	queue->initialPrimitiveGroupCount = 0;
	queue->transformCount = 0;

	queue->state = PRIMITIVE_SUBMISSION_QUEUE_STATE_RECORDING;
	return AS_SUCCESS;
}

void primQueueRecordCmds(asPrimitiveSubmissionQueue queue)
{
#if ASTRENGINE_VK
	/*Wait on Fence as Necesssary */ 
	/*Todo: Investigate performance cost... for now seems fine: per-thread, nanosecs, etc*/
	vkWaitForFences(asVkDevice, 1, &asVkInFlightFences[asVkCurrentFrame], VK_TRUE, UINT64_MAX);

	/*Update Continuous Recording*/
	asVkAllocation_t indexAlloc = asVkGetAllocFromBuffer(queue->offsetBuffs[queue->currentFrame]);
	asVkAllocation_t xformAlloc = asVkGetAllocFromBuffer(queue->transformBuffs[queue->currentFrame]);
	asVkFlushMemory(indexAlloc);
	asVkFlushMemory(xformAlloc);
	queue->currentFrame = asVkCurrentFrame;
#endif
	/*Allocate Command Buffers as Necessary*/
	for (int i = 0; i < AS_SCENE_RENDERPASS_COUNT; i++)
	{
		/*Category has been populated*/
		if (queue->renderpassPopulatedFlags & (1 << i))
		{
#if ASTRENGINE_VK
			const VkCommandBuffer* pVCmd = &queue->vSecondaryCommandBuffers[queue->currentFrame][i];
			if (*pVCmd == VK_NULL_HANDLE)
			{
				VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				allocInfo.commandPool = queue->vCommandPool;
				allocInfo.commandBufferCount = 1;
				if (vkAllocateCommandBuffers(asVkDevice, &allocInfo, pVCmd) != VK_SUCCESS) {
					asFatalError("vkAllocateCommandBuffers() Failed to create queue->vSecondaryCommandBuffers[]");
				}
			} else { 
				vkResetCommandBuffer(*pVCmd, 0);
			}

			/*Dimensions*/
			int32_t width, height;
			asGetRenderDimensions(0, true, &width, &height);

			/*Build Command Buffers*/
			recordSecondaryCommands(queue->initialPrimitiveGroupCount,
				queue->pInitialPrimGroups,
				AS_SCENE_RENDERPASS_SOLID,
				&mainSceneViewport,
				(float[]){(float)width, (float)height, 0.0f, 0.0f},
				*pVCmd,
				asVkCurrentFrame);
#endif
		}
	}
}

ASEXPORT asResults asSceneRendererSubmissionQueuePopulateEnd(asPrimitiveSubmissionQueue queue)
{
	/*Sort*/
	//Todo:

	/*Merge/Prune*/
	//Todo:

	/*Build Instance Transform Mappings*/
	uint32_t nextInstanceOffset = 0;
	for (uint32_t g = 0; g < queue->initialPrimitiveGroupCount; g++)
	{
		const uint16_t instanceCount = queue->pInitialPrimGroups[g].baseInstanceCount;
		const uint16_t transformOffset = queue->pInitialPrimGroups[g].transformOffset;
		const uint16_t transformOffsetPrev = queue->pInitialPrimGroups[g].transformOffsetPreviousFrame;
		for (uint16_t i = 0; i < instanceCount; i++)
		{
			queue->pInstanceTransformOffsets[nextInstanceOffset + i] = (struct primInstanceData){
				transformOffsetPrev + i,
				transformOffset + i,
			};
		}
		nextInstanceOffset += instanceCount;
	}
	queue->instanceCount = nextInstanceOffset;
	queue->state = PRIMITIVE_SUBMISSION_QUEUE_STATE_RECORDED;
	return AS_SUCCESS;
}

ASEXPORT asResults asSceneRendererSubmissionQueuePrepareFrameSubmit(asPrimitiveSubmissionQueue queue)
{
	/*Build Commands*/
	primQueueRecordCmds(queue);

	queue->state = PRIMITIVE_SUBMISSION_QUEUE_STATE_READY;
	return AS_SUCCESS;
}

ASEXPORT asResults asSceneRendererSubmissionAddBoundingBoxes(asPrimitiveSubmissionQueue queue, uint32_t boundsCount, float* pBoundingBoxes)
{
	for (size_t i = 0; i < boundsCount; i++)
	{
		glm_aabb_merge(queue->boundingBox, (vec3*)(pBoundingBoxes + i * 6), queue->boundingBox);
	}
	return AS_SUCCESS;
}

ASEXPORT asResults asSceneRendererSubmissionAddTransforms(asPrimitiveSubmissionQueue queue, uint32_t transformCount, asGfxInstanceTransform* pTransforms, uint32_t* pBeginningTransformOffset)
{
	if (queue->transformCount + transformCount > queue->transformMax) { return AS_FAILURE_OUT_OF_BOUNDS; }
	memcpy(queue->pTransforms + queue->transformCount, pTransforms, transformCount * sizeof(asGfxInstanceTransform));
	if (pBeginningTransformOffset) { *pBeginningTransformOffset = queue->transformCount; }
	queue->transformCount += transformCount;
	return AS_SUCCESS;
}

ASEXPORT asResults asSceneRendererSubmissionAddPrimitiveGroups(asPrimitiveSubmissionQueue queue, uint32_t primitiveCount, asGfxPrimativeGroupDesc* pDescs)
{
	if (queue->initialPrimitiveGroupCount + primitiveCount > queue->transformMax) { return AS_FAILURE_OUT_OF_BOUNDS; }
	memcpy(queue->pInitialPrimGroups + queue->initialPrimitiveGroupCount, pDescs, primitiveCount * sizeof(asGfxPrimativeGroupDesc));
	for (int i = 0; i < primitiveCount; i++) { queue->renderpassPopulatedFlags |= pDescs[i].renderPass; }
	queue->initialPrimitiveGroupCount += primitiveCount;
	return AS_SUCCESS;
}

/*Shader Settings Reflect Struct*/

#define REFLECT_MACRO_SceneShaderReflectData AS_REFLECT_STRUCT(asSceneShaderSettings,\
	AS_REFLECT_ENTRY_SINGLE(asSceneShaderSettings, float, lineWidth, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asSceneShaderSettings, float, backfaceCull, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asSceneShaderSettings, float, topologyMode, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asSceneShaderSettings, float, blendMode, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asSceneShaderSettings, float, depthWrite, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asSceneShaderSettings, float, depthTest, AS_REFLECT_FORMAT_NONE)\
)
REFLECT_MACRO_SceneShaderReflectData
#include "engine/common/reflection/asManualSerialList.h"
asReflectContainer SceneShaderReflectDataInstructions = REFLECT_MACRO_SceneShaderReflectData

/*Fill out Vulkan Pipelines for Scene*/
ASEXPORT asResults _asFillGfxPipeline_Scene(
	asBinReader* pShaderAsBin,
	asGfxAPIs api,
	asPipelineType pipelineType,
	void* pDesc,
	const char* pipelineName,
	asPipelineHandle* pPipelineOut,
	void* pUserData)
{
	/*Load Settings from Shader*/
	asSceneShaderSettings settings = { 0 };
	settings.lineWidth = 1.0f;
	settings.backfaceCull = 1;
	settings.topologyMode = 0;
	settings.depthWrite = 1;
	settings.depthTest = 1;
	settings.blendMode = 0;
	char* reflectData = NULL;
	size_t reflectDataSize = 0;
	asHash32_t sectionNameHash = asHashBytes32_xxHash("asSceneShaderSettings", 21);
	asBinReaderGetSection(pShaderAsBin, (asBinSectionIdentifier) {"BLOB", sectionNameHash}, &reflectData, &reflectDataSize);
	asReflectLoadFromBinary(&settings, sizeof(settings), 1, &SceneShaderReflectDataInstructions, reflectData, reflectDataSize, NULL, NULL);

	/*Create Pipeline*/
#if ASTRENGINE_VK
	if (api != AS_GFXAPI_VULKAN || pipelineType != AS_PIPELINETYPE_GRAPHICS) { return AS_FAILURE_UNKNOWN_FORMAT; }
	VkGraphicsPipelineCreateInfo* pGraphicsPipelineDesc = (VkGraphicsPipelineCreateInfo*)pDesc;
	pGraphicsPipelineDesc->basePipelineHandle = VK_NULL_HANDLE;
	pGraphicsPipelineDesc->basePipelineIndex = 0;

	/*Color Blending*/
	VkPipelineColorBlendAttachmentState attachmentBlends[] = {
		settings.blendMode? AS_VK_INIT_HELPER_ATTACHMENT_BLEND_MIX(VK_TRUE) : AS_VK_INIT_HELPER_ATTACHMENT_BLEND_MIX(VK_FALSE)
	};
	pGraphicsPipelineDesc->pColorBlendState = &AS_VK_INIT_HELPER_PIPE_COLOR_BLEND_STATE(ASARRAYLEN(attachmentBlends), attachmentBlends);
	/*Depth Stencil*/
	pGraphicsPipelineDesc->pDepthStencilState = &AS_VK_INIT_HELPER_PIPE_DEPTH_STENCIL_STATE(
		settings.depthTest? VK_TRUE : VK_FALSE,
		settings.depthWrite? VK_TRUE : VK_FALSE,
		VK_COMPARE_OP_GREATER, VK_FALSE, 0.0f, 1.0f);
	/*Rasterizer*/
	pGraphicsPipelineDesc->pRasterizationState = &AS_VK_INIT_HELPER_PIPE_RASTERIZATION_STATE(
		settings.topologyMode == 0 ? VK_POLYGON_MODE_FILL:
		(settings.topologyMode == 1 ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_POINT),
		settings.backfaceCull? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE,
		VK_FRONT_FACE_CLOCKWISE,
		VK_FALSE,
		0.0f, 0.0f, 0.0f, settings.lineWidth);
	/*Input Assembler*/
	VkPrimitiveTopology vkTopology;
	switch ((int)settings.topologyMode)
	{
	default: vkTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
	case 1: vkTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
	case 2: vkTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
	}
	pGraphicsPipelineDesc->pInputAssemblyState = &AS_VK_INIT_HELPER_PIPE_INPUT_ASSEMBLER_STATE(topologyMode, VK_FALSE);
	/*MSAA*/
	pGraphicsPipelineDesc->pMultisampleState = &AS_VK_INIT_HELPER_PIPE_MSAA_STATE_NONE();
	/*Tess*/
	pGraphicsPipelineDesc->pTessellationState = NULL;
	/*Viewport*/
	pGraphicsPipelineDesc->pViewportState = &AS_VK_INIT_HELPER_PIPE_VIEWPORT_STATE_EVERYTHING(0, 0);
	/*Dynamic States*/
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, VK_DYNAMIC_STATE_DEPTH_BOUNDS, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK
	};
	pGraphicsPipelineDesc->pDynamicState = &AS_VK_INIT_HELPER_PIPE_DYNAMIC_STATE(ASARRAYLEN(dynamicStates), dynamicStates);

	/*Vertex Inputs*/
	size_t vtxSize = sizeof(asVertexGeneric);
	size_t posOffset = offsetof(asVertexGeneric, position);
	size_t normalOffset = offsetof(asVertexGeneric, normal);
	size_t tangentOffset = offsetof(asVertexGeneric, tangent);
	size_t uv0Offset = offsetof(asVertexGeneric, uv0);
	size_t uv1Offset = offsetof(asVertexGeneric, uv1);
	size_t colorOffset = offsetof(asVertexGeneric, color);
	size_t boneIdxOffset = offsetof(asVertexGeneric, boneIdx); /*Set to 0 for non-skined*/
	size_t boneWeightOffset = offsetof(asVertexGeneric, boneWeight);
	VkVertexInputBindingDescription vertexBindings[] = {
		AS_VK_INIT_HELPER_VERTEX_BINDING(0, vtxSize, VK_VERTEX_INPUT_RATE_VERTEX)
	};
	VkVertexInputAttributeDescription vertexAttributes[] = {
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(0, 0, VK_FORMAT_R32G32B32_SFLOAT, posOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(1, 0, VK_FORMAT_A2R10G10B10_SNORM_PACK32, normalOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(2, 0, VK_FORMAT_A2R10G10B10_SNORM_PACK32, tangentOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(3, 0, VK_FORMAT_R16G16_SFLOAT, uv0Offset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(4, 0, VK_FORMAT_R16G16_SFLOAT, uv1Offset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(5, 0, VK_FORMAT_R8G8B8A8_UNORM, colorOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(6, 0, VK_FORMAT_R16G16B16A16_UINT, boneIdxOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(7, 0, VK_FORMAT_R8G8B8A8_UNORM, boneWeightOffset),
	};
	pGraphicsPipelineDesc->pVertexInputState = &AS_VK_INIT_HELPER_PIPE_VERTEX_STATE(
		ASARRAYLEN(vertexBindings), vertexBindings,
		ASARRAYLEN(vertexAttributes), vertexAttributes);

	pGraphicsPipelineDesc->renderPass = sceneRenderPass;
	pGraphicsPipelineDesc->subpass = 0;
	pGraphicsPipelineDesc->layout = scenePipelineLayout;

	if (vkCreateGraphicsPipelines(asVkDevice, VK_NULL_HANDLE, 1, pGraphicsPipelineDesc, AS_VK_MEMCB, (VkPipeline*)pPipelineOut) != VK_SUCCESS)
		asFatalError("vkCreateGraphicsPipelines() Failed to create scenePipeline");
	return AS_SUCCESS;
#endif
	return AS_FAILURE_UNKNOWN;
}

void _createScreenResources()
{
	/*Renderpass*/
	{
		VkAttachmentDescription attachments[] = {
			(VkAttachmentDescription) { /*Color Buffer*/
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			},
			(VkAttachmentDescription) { /*Depth Buffer*/
				.format = VK_FORMAT_D32_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			},
		};

		/*Setup Subpass*/
		VkSubpassDescription subpass = (VkSubpassDescription){ 0 };
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &(VkAttachmentReference) {
			.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		subpass.pDepthStencilAttachment = &(VkAttachmentReference) {
			.attachment = 1,
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		VkRenderPassCreateInfo createInfo = (VkRenderPassCreateInfo){ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		createInfo.attachmentCount = ASARRAYLEN(attachments);
		createInfo.pAttachments = attachments;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(asVkDevice, &createInfo, AS_VK_MEMCB, &sceneRenderPass) != VK_SUCCESS)
			asFatalError("vkCreateRenderPass() Failed to create pScreen->simpleRenderPass");
	}
	/*Framebuffer*/
	{
		asVkScreenResources* pScreen = asVkGetScreenResourcesPtr(0);
		VkImageView attachments[] = {
			asVkGetViewFromTexture(pScreen->compositeTexture),
			asVkGetViewFromTexture(pScreen->depthTexture)
		};
		VkFramebufferCreateInfo createInfo = (VkFramebufferCreateInfo){ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = sceneRenderPass;
		createInfo.attachmentCount = ASARRAYLEN(attachments);
		createInfo.pAttachments = attachments;
		createInfo.width = pScreen->extents.width;
		createInfo.height = pScreen->extents.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(asVkDevice, &createInfo, AS_VK_MEMCB, &sceneFramebuffer) != VK_SUCCESS)
			asFatalError("vkCreateFramebuffer() Failed to create simpleFrameBuffer");
	}
}

void _destroyScreenResources()
{
	vkDestroyRenderPass(asVkDevice, sceneRenderPass, AS_VK_MEMCB);
	vkDestroyFramebuffer(asVkDevice, sceneFramebuffer, AS_VK_MEMCB);
}

asPrimitiveSubmissionQueue debugDrawPrimQueue;

ASEXPORT asResults asInitSceneRenderer()
{
#if ASTRENGINE_VK
	/*Descriptor Set Layout*/
	{
		VkDescriptorSetLayoutCreateInfo desc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		desc.bindingCount = 1;

		VkDescriptorSetLayoutBinding uboBinding = { 0 };
		uboBinding.binding = AS_BINDING_VIEWER_UBO;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags = VK_SHADER_STAGE_ALL;
		uboBinding.descriptorCount = 1;
		desc.pBindings = &uboBinding;
		if (vkCreateDescriptorSetLayout(asVkDevice, &desc, AS_VK_MEMCB, &sceneViewDescSetLayout) != VK_SUCCESS)
			asFatalError("vkCreateDescriptorSetLayout() Failed to create sceneViewDescSetLayout");
	}
	/*Setup Pipeline Layout*/
	{
		VkDescriptorSetLayout descSetLayouts[2] = { 0 };
		asVkGetTexturePoolDescSetLayout(&descSetLayouts[0]);
		descSetLayouts[1] = sceneViewDescSetLayout;

		VkPipelineLayoutCreateInfo createInfo = (VkPipelineLayoutCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = ASARRAYLEN(descSetLayouts);
		createInfo.pSetLayouts = descSetLayouts;
		createInfo.pushConstantRangeCount = 1;
		VkPushConstantRange pcRange;
		pcRange.offset = 0;
		pcRange.size = sizeof(struct scenePushConstants);
		pcRange.stageFlags = VK_SHADER_STAGE_ALL;
		createInfo.pPushConstantRanges = &pcRange;
		if (vkCreatePipelineLayout(asVkDevice, &createInfo, AS_VK_MEMCB, &scenePipelineLayout) != VK_SUCCESS)
			asFatalError("vkCreatePipelineLayout() Failed to create imGuiPipelineLayout");
	}
	/*Descriptor Pool*/
	{
		VkDescriptorPoolSize fontImagePoolSize = { 0 };
		fontImagePoolSize.descriptorCount = SCENE_MAX_DESC_SETS;
		fontImagePoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorPoolCreateInfo createInfo = (VkDescriptorPoolCreateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.maxSets = SCENE_MAX_DESC_SETS;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &fontImagePoolSize;
		if (vkCreateDescriptorPool(asVkDevice, &createInfo, AS_VK_MEMCB, &sceneDescriptorPool) != VK_SUCCESS)
			asFatalError("vkCreateDescriptorPool() Failed to create sceneDescriptorPool");
	}

	_createScreenResources();
	sceneViewport_Init(&mainSceneViewport);
#endif

	/*Debug Drawing*/
	{
		asPrimitiveSubmissionQueueDesc desc = {
			.viewportIdx = 0,
			.maxTransforms = 1,
			.maxPrimitives = 1,
			.maxInstances = 1,
		};
		debugDrawPrimQueue = asSceneRendererCreateSubmissionQueue(&desc);
	}

	return AS_SUCCESS;
}

ASEXPORT asResults asTriggerResizeSceneRenderer()
{
	_destroyScreenResources();
	_createScreenResources();
	for (int i = 0; i < arrlen(mainSceneViewport.pPrimQueues); i++)
	{
		const struct asPrimitiveSubmissionQueueT* pPrimQueue = mainSceneViewport.pPrimQueues[i];
		primQueueRecordCmds(pPrimQueue);
	}
	return AS_SUCCESS;
}

void executeVpSubmissionQueues(VkCommandBuffer vCmd, struct sceneViewport* viewport, int renderpass)
{
	for (int i = 0; i < arrlen(viewport->pPrimQueues); i++)
	{
		const struct asPrimitiveSubmissionQueueT* pPrimQueue = viewport->pPrimQueues[i];
		if (pPrimQueue->state == PRIMITIVE_SUBMISSION_QUEUE_STATE_READY &&
			pPrimQueue->vSecondaryCommandBuffers[asVkCurrentFrame][renderpass] != VK_NULL_HANDLE) {
#if ASTRENGINE_VK
			vkCmdExecuteCommands(vCmd, 1, &pPrimQueue->vSecondaryCommandBuffers[asVkCurrentFrame][renderpass]);
#endif
		}
	}
}

void resetVpSubmissionQueues(struct sceneViewport* viewport)
{
	for (int i = 0; i < arrlen(viewport->pPrimQueues); i++)
	{
		struct asPrimitiveSubmissionQueueT* pPrimQueue = viewport->pPrimQueues[i];
		if (pPrimQueue->state == PRIMITIVE_SUBMISSION_QUEUE_STATE_READY)
		{
			pPrimQueue->state = PRIMITIVE_SUBMISSION_QUEUE_STATE_RECORDED;
		}
	}
}

ASEXPORT asResults asSceneRendererUploadDebugDraws(int32_t viewport)
{
	/*Draw Debug Lines for Viewport via ImGui*/
	_asDebugDrawSecureLists();
	asDebugDrawLineData* lineData;
	size_t lineCount = _asDebugDrawGetLineList(&lineData);
	asSceneRendererSubmissionQueuePopulateBegin(debugDrawPrimQueue);

	for (size_t i = 0; i < lineCount; i++)
	{
		const asDebugDrawLineData line = lineData[i];
		vec3* inVerts = line.start;

		/*Convert Verts*/
		for (int i = 0; i < 2; i++)
		{
			/*Upload*/
		}
		
	}

	/*Transform*/
	asGfxInstanceTransform xform = { 0 };
	asSceneRendererSubmissionAddTransforms(debugDrawPrimQueue, 1, &xform, 0);

	/*Prim*/
	asGfxPrimativeGroupDesc prim = { 0 };
	prim.indexBuffer = asHandle_Invalidate();
	prim.baseInstanceCount = 1;
	prim.vertexCount = lineCount * 2;
	asSceneRendererSubmissionAddPrimitiveGroups(debugDrawPrimQueue, 1, &prim);

	asSceneRendererSubmissionQueuePopulateEnd(debugDrawPrimQueue);
	asSceneRendererSubmissionQueuePrepareFrameSubmit(debugDrawPrimQueue);

	_asDebugDrawResetLineList();
	_asDebugDrawUnlockLists();
	return AS_SUCCESS;
}

ASEXPORT asResults asSceneRendererDraw(int32_t viewport)
{
	/*Upload Scene Viewports to GPU*/
	sceneViewport_FlushGPU(&mainSceneViewport);

#if ASTRENGINE_VK
	/*Begin Command Buffer*/
	VkCommandBuffer vCmd = asVkGetNextGraphicsCommandBuffer();
	VkCommandBufferBeginInfo cmdInfo = (VkCommandBufferBeginInfo){ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(vCmd, &cmdInfo);

	/*Begin Render Pass*/
	VkClearValue clearValues[] = {
		(VkClearValue) {
			.color = {0.0f,0.0f,0.0f,1.0f}
		},
		(VkClearValue) {
			.depthStencil = {0.0f, 0}
		}
	};

	int32_t width, height;
	asGetRenderDimensions(0, true, &width, &height);

	VkRenderPassBeginInfo beginInfo = (VkRenderPassBeginInfo){ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.clearValueCount = ASARRAYLEN(clearValues);
	beginInfo.pClearValues = clearValues;
	beginInfo.renderArea = (VkRect2D){ {0,0},{width, height} };
	beginInfo.renderPass = sceneRenderPass;
	beginInfo.framebuffer = sceneFramebuffer;
	vkCmdBeginRenderPass(vCmd, &beginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	executeVpSubmissionQueues(vCmd, &mainSceneViewport, 2);

	vkCmdEndRenderPass(vCmd);
	vkEndCommandBuffer(vCmd);
#endif
	/*Invalidate all command buffers for frame*/
	resetVpSubmissionQueues(&mainSceneViewport);
	return AS_SUCCESS;
}

ASEXPORT asResults asShutdownSceneRenderer()
{
	asSceneRendererDestroySubmissionQueue(debugDrawPrimQueue);

	sceneViewport_Destroy(&mainSceneViewport);
#if ASTRENGINE_VK
	vkDestroyDescriptorPool(asVkDevice, sceneDescriptorPool, AS_VK_MEMCB);
	vkDestroyPipelineLayout(asVkDevice, scenePipelineLayout, AS_VK_MEMCB);
	vkDestroyDescriptorSetLayout(asVkDevice, sceneViewDescSetLayout, AS_VK_MEMCB);
#endif
	_destroyScreenResources();
	return AS_SUCCESS;
}