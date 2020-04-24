#include "asSceneRenderer.h"

#include "asBindlessTexturePool.h"
#if ASTRENGINE_VK
#include "../renderer/vulkan/asVulkanBackend.h"
#include <SDL_vulkan.h>

VkPipelineLayout scenePipelineLayout;
VkRenderPass sceneRenderPass;
VkFramebuffer sceneFramebuffer;
#endif

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
#if ASTRENGINE_VK
	if (api != AS_GFXAPI_VULKAN || pipelineType != AS_PIPELINETYPE_GRAPHICS) { return AS_FAILURE_UNKNOWN_FORMAT; }
	VkGraphicsPipelineCreateInfo* pGraphicsPipelineDesc = (VkGraphicsPipelineCreateInfo*)pDesc;
	pGraphicsPipelineDesc->basePipelineHandle = VK_NULL_HANDLE;
	pGraphicsPipelineDesc->basePipelineIndex = 0;

	/*Color Blending*/
	VkPipelineColorBlendAttachmentState attachmentBlends[] = {
		AS_VK_INIT_HELPER_ATTACHMENT_BLEND_MIX(VK_TRUE)
	};
	pGraphicsPipelineDesc->pColorBlendState = &AS_VK_INIT_HELPER_PIPE_COLOR_BLEND_STATE(ASARRAYLEN(attachmentBlends), attachmentBlends);
	/*Depth Stencil*/
	pGraphicsPipelineDesc->pDepthStencilState = &AS_VK_INIT_HELPER_PIPE_DEPTH_STENCIL_STATE(
		VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS, VK_FALSE, 0.0f, 1.0f);
	/*Rasterizer*/
	pGraphicsPipelineDesc->pRasterizationState = &AS_VK_INIT_HELPER_PIPE_RASTERIZATION_STATE(
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_CLOCKWISE,
		VK_FALSE,
		0.0f, 0.0f, 0.0f, 1.0f);
	/*Input Assembler*/
	pGraphicsPipelineDesc->pInputAssemblyState = &AS_VK_INIT_HELPER_PIPE_INPUT_ASSEMBLER_STATE(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
	/*MSAA*/
	pGraphicsPipelineDesc->pMultisampleState = &AS_VK_INIT_HELPER_PIPE_MSAA_STATE_NONE();
	/*Tess*/
	pGraphicsPipelineDesc->pTessellationState = NULL;
	/*Viewport*/
	pGraphicsPipelineDesc->pViewportState = &AS_VK_INIT_HELPER_PIPE_VIEWPORT_STATE_EVERYTHING(0, 0);
	/*Dynamic States*/
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, VK_DYNAMIC_STATE_DEPTH_BOUNDS
	};
	pGraphicsPipelineDesc->pDynamicState = &AS_VK_INIT_HELPER_PIPE_DYNAMIC_STATE(ASARRAYLEN(dynamicStates), dynamicStates);

	/*Vertex Inputs*/
	size_t vtxSize = 1;
	size_t posOffset = 0;
	size_t normalOffset = 0;
	size_t tangentOffset = 0;
	size_t uv0Offset = 0;
	size_t uv1Offset = 0;
	size_t colorOffset = 0;
	size_t boneIdxOffset = 0; /*Set to 0 for non-skined*/
	size_t boneWeightOffset = 0;
	VkVertexInputBindingDescription vertexBindings[] = {
		AS_VK_INIT_HELPER_VERTEX_BINDING(0, vtxSize, VK_VERTEX_INPUT_RATE_VERTEX)
	};
	VkVertexInputAttributeDescription vertexAttributes[] = {
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(0, 0, VK_FORMAT_R32G32B32_SFLOAT, posOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(1, 0, VK_FORMAT_B10G11R11_UFLOAT_PACK32, normalOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(2, 0, VK_FORMAT_A2R10G10B10_UNORM_PACK32, tangentOffset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(3, 0, VK_FORMAT_R16G16_SFLOAT, uv0Offset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(4, 0, VK_FORMAT_R16G16_SFLOAT, uv1Offset),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(5, 0, VK_FORMAT_R8G8B8A8_SNORM, colorOffset),
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

ASEXPORT asResults asInitSceneRenderer()
{
#if ASTRENGINE_VK
	/*Setup Pipeline Layout*/
	{
		VkDescriptorSetLayout descSetLayouts[1] = { 0 };
		asVkGetTexturePoolDescSetLayout(&descSetLayouts[0]);
		//Todo: Other Desc Sets

		VkPipelineLayoutCreateInfo createInfo = (VkPipelineLayoutCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = ASARRAYLEN(descSetLayouts);
		createInfo.pSetLayouts = descSetLayouts;
		if (vkCreatePipelineLayout(asVkDevice, &createInfo, AS_VK_MEMCB, &scenePipelineLayout) != VK_SUCCESS)
			asFatalError("vkCreatePipelineLayout() Failed to create imGuiPipelineLayout");
	}
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
			asFatalError("vkCreateFramebuffer() Failed to create pScreen->simpleFrameBuffer");
	}
#endif
	return AS_SUCCESS;
}

ASEXPORT asResults asSceneRendererDraw(int32_t viewport)
{
	int viewWidth, viewHeight;
	asGetRenderDimensions(viewport, true, &viewWidth, &viewHeight);

#if ASTRENGINE_VK
	/*Begin Command Buffer*/
	VkCommandBuffer vCmd = asVkGetNextGraphicsCommandBuffer();
	VkCommandBufferBeginInfo cmdInfo = (VkCommandBufferBeginInfo){ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(vCmd, &cmdInfo);

	/*Begin Render Pass*/
	VkClearValue clearValues[] = {
		(VkClearValue) {
			.color = {0.0f,0.0f,1.0f,1.0f}
		},
		(VkClearValue) {
			.depthStencil = {1.0f, 0}
		}
	};
	VkRenderPassBeginInfo beginInfo = (VkRenderPassBeginInfo){ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.clearValueCount = ASARRAYLEN(clearValues);
	beginInfo.pClearValues = clearValues;
	beginInfo.renderArea = (VkRect2D){ {0,0},{viewWidth, viewHeight} };
	beginInfo.renderPass = sceneRenderPass;
	beginInfo.framebuffer = sceneFramebuffer;
	vkCmdBeginRenderPass(vCmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	/*Setup Viewport/Scissors*/
	VkViewport vkViewport = (VkViewport){ 0 };
	vkViewport.maxDepth = 1.0f;
	vkViewport.width = (float)viewWidth;
	vkViewport.height = (float)viewHeight;
	vkCmdSetViewport(vCmd, 0, 1, &vkViewport);
	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = viewWidth;
	scissor.extent.height = viewHeight;
	vkCmdSetScissor(vCmd, 0, 1, &scissor);
#endif

	//Todo: Draw Scene

	//for(int terrainId = 0; i < terrainCount; terrainId++)
	//{
#if ASTRENGINE_VK
		/*VkBuffer vVertexBuffer = asVkGetBufferFromBuffer(imGuiVertexBuffer[asVkCurrentFrame]);
		VkBuffer vIndexBuffer = asVkGetBufferFromBuffer(imGuiIndexBuffer[asVkCurrentFrame]);
		VkDeviceSize vtxOffset = 0;
		vkCmdBindVertexBuffers(vCmd, 0, 1, &vVertexBuffer, &vtxOffset);
		vkCmdBindIndexBuffer(vCmd, vIndexBuffer, 0, VK_INDEX_TYPE_UINT16);*/
		/*Bind shader pipeline*/
		/*vkCmdBindPipeline(vCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)imGuiShaderFx.pipelines[0]);
		asTexturePoolBindCmd(AS_GFXAPI_VULKAN, &vCmd, &imGuiPipelineLayout);*/
#endif
	//}

#if ASTRENGINE_VK
	vkCmdEndRenderPass(vCmd);
	vkEndCommandBuffer(vCmd);
#endif
	return AS_SUCCESS;
}

ASEXPORT asResults asShutdownSceneRenderer()
{
#if ASTRENGINE_VK
	vkDestroyRenderPass(asVkDevice, sceneRenderPass, AS_VK_MEMCB);
	vkDestroyFramebuffer(asVkDevice, sceneFramebuffer, AS_VK_MEMCB);
#endif
	return AS_SUCCESS;
}