#include "asNuklearImplimentation.h"

/*Todo: Replace in favor of IMGUI... Nuklear Development has stopped*/

#if ASTRENGINE_NUKLEAR
#define NK_IMPLEMENTATION
#include "nuklear/nuklear.h"

#include "SDL_events.h"

#if ASTRENGINE_VK
#include "../renderer/vulkan/asVulkanBackend.h"
#include <SDL_vulkan.h>
#endif

#define AS_NK_MAX_MEMORY 500000
#define AS_NK_MAX_VERTS 65536
#define AS_NK_MAX_INDICES 65536
struct nk_context nkContext;
void* nkMemory;

struct nk_font_atlas nkFontAtlas;
asTextureHandle_t nkFontTexture;
struct nk_draw_null_texture nkNullDraw;

struct nk_buffer nkCommands;

asBufferHandle_t nkVertexBuffer[AS_MAX_INFLIGHT];
asBufferHandle_t nkIndexBuffer[AS_MAX_INFLIGHT];

asShaderFx nkShaderFx;

#define AS_BINDING_GUI_TEXTURE 300

#if ASTRENGINE_VK
VkDescriptorSetLayout vkNuklearDescriptorSetLayout;
VkPipelineLayout vkNuklearPipelineLayout;
VkDescriptorPool vkNuklearDescriptorPool;

/*Descriptor Set for the Default Font*/
VkDescriptorSet vkNuklearDefaultFontDescriptorSet;
#endif

struct nkVertex
{
	float pos[2];
	float uv[2];
	unsigned char rgba[4];
};

ASEXPORT struct nk_context* asGetNuklearContextPtr()
{
	return &nkContext;
}

#if ASTRENGINE_VK
ASEXPORT asResults _asFillGfxPipeline_Nuklear(
	asBinReader* pShaderAsBin,
	asGfxAPIs api,
	asPipelineType pipelineType,
	void* pDesc,
	const char* pipelineName,
	asPipelineHandle* pPipelineOut,
	void* pUserData)
{
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
	VkVertexInputBindingDescription vertexBindings[] = { 
		AS_VK_INIT_HELPER_VERTEX_BINDING(0, sizeof(struct nkVertex), VK_VERTEX_INPUT_RATE_VERTEX)
	};
	VkVertexInputAttributeDescription vertexAttributes[] = { 
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(struct nkVertex, pos)),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(struct nkVertex, uv)),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(struct nkVertex, rgba))
	};
	pGraphicsPipelineDesc->pVertexInputState = &AS_VK_INIT_HELPER_PIPE_VERTEX_STATE(
		ASARRAYLEN(vertexBindings), vertexBindings,
		ASARRAYLEN(vertexAttributes), vertexAttributes);

	pGraphicsPipelineDesc->renderPass = asVkGetSimpleDrawingRenderpass();
	pGraphicsPipelineDesc->subpass = 0;
	pGraphicsPipelineDesc->layout = vkNuklearPipelineLayout;

	if(vkCreateGraphicsPipelines(asVkDevice, VK_NULL_HANDLE, 1, pGraphicsPipelineDesc, AS_VK_MEMCB, (VkPipeline*)pPipelineOut) != VK_SUCCESS)
		asFatalError("vkCreateGraphicsPipelines() Failed to create nkPipeline");
	return AS_SUCCESS;
}

/*Vulkan allows mappings to stay persistant*/
void* vVertexBufferBindings[AS_MAX_INFLIGHT];
void* vIndexBufferBindings[AS_MAX_INFLIGHT];
#endif
ASEXPORT void asInitNk()
{
	/*Font*/
	const void* img;
	int w, h;
	nk_font_atlas_init_default(&nkFontAtlas);
	img = nk_font_atlas_bake(&nkFontAtlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	/*Create font texture*/
	{
		asTextureDesc_t desc = asTextureDesc_Init();
		desc.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
		desc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		desc.format = AS_COLORFORMAT_RGBA8_UNORM;
		desc.width = w;
		desc.height = h;
		desc.initialContentsBufferSize = w * h * 4;
		desc.initialContentsRegionCount = 1;
		asTextureContentRegion_t region = (asTextureContentRegion_t) { 0 };
		region.bufferStart = 0;
		region.extent[0] = desc.width;
		region.extent[1] = desc.height;
		region.extent[2] = 1;
		region.layerCount = 1;
		region.layer = 0;
		region.mipLevel = 0;
		desc.pInitialContentsRegions = &region;
		desc.pInitialContentsBuffer = img;
		desc.pDebugLabel = "NuklearFont";
		nkFontTexture = asCreateTexture(&desc);
	}
	/*Create vertex buffer*/
	{
		asBufferDesc_t desc = asBufferDesc_Init();
		desc.bufferSize = AS_NK_MAX_VERTS * sizeof(struct nkVertex);
		desc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
		desc.usageFlags = AS_BUFFERUSAGE_VERTEX;
		desc.initialContentsBufferSize = desc.bufferSize;
		desc.pDebugLabel = "NuklearVerts";
		for(int i = 0; i < AS_MAX_INFLIGHT; i++)
			nkVertexBuffer[i] = asCreateBuffer(&desc);
	}
	/*Create index buffer*/
	{
		asBufferDesc_t desc = asBufferDesc_Init();
		desc.bufferSize = AS_NK_MAX_INDICES * sizeof(uint16_t);
		desc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
		desc.usageFlags = AS_BUFFERUSAGE_INDEX;
		desc.initialContentsBufferSize = desc.bufferSize;
		desc.pDebugLabel = "NuklearIndices";
		for (int i = 0; i < AS_MAX_INFLIGHT; i++)
			nkIndexBuffer[i] = asCreateBuffer(&desc);
	}
#if ASTRENGINE_VK
	/*Map persistent memory*/
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		asVkAllocation_t vertAlloc = asVkGetAllocFromBuffer(nkVertexBuffer[i]);
		asVkAllocation_t idxAlloc = asVkGetAllocFromBuffer(nkIndexBuffer[i]);
		asVkMapMemory(vertAlloc, 0, vertAlloc.size, &vVertexBufferBindings[i]);
		asVkMapMemory(idxAlloc, 0, idxAlloc.size, &vIndexBufferBindings[i]);
	}
	/*Setup Pipeline Layout*/
	{
		VkDescriptorSetLayoutBinding descriptorBindings[] = {
			(VkDescriptorSetLayoutBinding){
				.binding = AS_BINDING_GUI_TEXTURE,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.pImmutableSamplers = asVkGetSimpleSamplerPtr(true),
				.stageFlags = VK_SHADER_STAGE_ALL,
			}
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetLayoutInfo.bindingCount = ASARRAYLEN(descriptorBindings);
		descriptorSetLayoutInfo.pBindings = descriptorBindings;
		if (vkCreateDescriptorSetLayout(asVkDevice, &descriptorSetLayoutInfo, AS_VK_MEMCB, &vkNuklearDescriptorSetLayout) != VK_SUCCESS)
			asFatalError("vkCreateDescriptorSetLayout() Failed to create vkNuklearDescriptorSetLayout");

		VkPipelineLayoutCreateInfo createInfo = (VkPipelineLayoutCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = 1;
		createInfo.pSetLayouts = &vkNuklearDescriptorSetLayout;
		if (vkCreatePipelineLayout(asVkDevice, &createInfo, AS_VK_MEMCB, &vkNuklearPipelineLayout) != VK_SUCCESS)
			asFatalError("vkCreatePipelineLayout() Failed to create vkNuklearPipelineLayout");
	}
	/*Descriptor Pool*/
	{
		VkDescriptorPoolSize fontImagePoolSize = { 0 };
		fontImagePoolSize.descriptorCount = 1;
		fontImagePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VkDescriptorPoolCreateInfo createInfo = (VkDescriptorPoolCreateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &fontImagePoolSize;
		if (vkCreateDescriptorPool(asVkDevice, &createInfo, AS_VK_MEMCB, &vkNuklearDescriptorPool) != VK_SUCCESS)
			asFatalError("vkCreateDescriptorPool() Failed to create vkNuklearDescriptorPool");
	}
	/*Descriptor Set*/
	{
		VkDescriptorSetAllocateInfo descSetAllocInfo = (VkDescriptorSetAllocateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descSetAllocInfo.descriptorPool = vkNuklearDescriptorPool;
		descSetAllocInfo.descriptorSetCount = 1;
		descSetAllocInfo.pSetLayouts = &vkNuklearDescriptorSetLayout;
		if (vkAllocateDescriptorSets(asVkDevice, &descSetAllocInfo, &vkNuklearDefaultFontDescriptorSet) != VK_SUCCESS)
			asFatalError("vkAllocateDescriptorSets() Failed to allocate vkNuklearDefaultFontDescriptorSet");

		VkWriteDescriptorSet descSetWrite = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descSetWrite.dstSet = vkNuklearDefaultFontDescriptorSet;
		descSetWrite.dstBinding = AS_BINDING_GUI_TEXTURE;
		descSetWrite.descriptorCount = 1;
		descSetWrite.dstArrayElement = 0;
		descSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descSetWrite.pImageInfo = &(VkDescriptorImageInfo) {
			.sampler = *asVkGetSimpleSamplerPtr(true),
			.imageView = asVkGetViewFromTexture(nkFontTexture),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		vkUpdateDescriptorSets(asVkDevice, 1, &descSetWrite, 0, NULL);
	}
#endif
	/*Load shader*/
	{
		unsigned char* fileData;
		size_t fileSize;
		asResourceLoader_t resourceLoader;
		asResourceFileID_t resourceFileId;
		asResourceLoader_OpenByPath(&resourceLoader, &resourceFileId, "shaders/core/Nuklear_FX.asfx", 28);
		fileSize = asResourceLoader_GetContentSize(&resourceLoader);
		fileData = asMalloc(fileSize);
		asResourceLoader_ReadAll(&resourceLoader, fileSize, fileData);
		asResourceLoader_Close(&resourceLoader);

		asBinReader shaderBin;
		asBinReaderOpenMemory(&shaderBin, "ASFX", fileData, fileSize);
		asCreateShaderFx(&shaderBin, &nkShaderFx, AS_QUALITY_HIGH);
		asFree(fileData);
	}

	nk_font_atlas_end(&nkFontAtlas, nk_handle_id(nkFontTexture._index), &nkNullDraw);

	/*Context*/
	nkMemory = asMalloc(AS_NK_MAX_MEMORY);
	memset(nkMemory, 0, AS_NK_MAX_MEMORY);
	nk_init_fixed(&nkContext, nkMemory, AS_NK_MAX_MEMORY, &nkFontAtlas.default_font->handle);
	/*Create nuklear command list*/
	nk_buffer_init_default(&nkCommands);
}

ASEXPORT void asNkDraw(int32_t viewport)
{
	const struct nk_draw_command* nkCmd;
	int viewWidth, viewHeight;
	asGetRenderDimensions(0, true, &viewWidth, &viewHeight);

	struct nk_buffer nkVerts, nkElements;
	/*Convert to vertices*/
	{
		struct nk_convert_config config = (struct nk_convert_config) { 0 };
		static const struct nk_draw_vertex_layout_element vertexLayout[] = {
			{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(struct nkVertex, pos)},
			{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(struct nkVertex, uv)},
			{NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(struct nkVertex, rgba)},
			{NK_VERTEX_LAYOUT_END}
		};
		config.vertex_layout = vertexLayout;
		config.vertex_size = sizeof(struct nkVertex);
		config.vertex_alignment = NK_ALIGNOF(struct nkVertex);
		config.null = nkNullDraw;
		config.circle_segment_count = 22;
		config.curve_segment_count = 22;
		config.arc_segment_count = 22;
		config.global_alpha = 1.0f;
		config.shape_AA = NK_ANTI_ALIASING_OFF;
		config.line_AA = NK_ANTI_ALIASING_OFF;
		nk_buffer_init_fixed(&nkVerts, vVertexBufferBindings[asVkCurrentFrame], AS_NK_MAX_VERTS * sizeof(struct nkVertex));
		nk_buffer_init_fixed(&nkElements, vIndexBufferBindings[asVkCurrentFrame], AS_NK_MAX_INDICES * sizeof(uint16_t));
		nk_convert(&nkContext, &nkCommands, &nkVerts, &nkElements, &config);
#if ASTRENGINE_VK
		asVkAllocation_t vertAlloc = asVkGetAllocFromBuffer(nkVertexBuffer[asVkCurrentFrame]);
		asVkAllocation_t idxAlloc = asVkGetAllocFromBuffer(nkIndexBuffer[asVkCurrentFrame]);
		asVkFlushMemory(vertAlloc);
		asVkFlushMemory(idxAlloc);
#endif
	}

	/*Render all draws*/
#if ASTRENGINE_VK
	/*Bind vertex/index buffers*/
	VkCommandBuffer vCmd = asVkGetNextGraphicsCommandBuffer();
	VkCommandBufferBeginInfo cmdInfo = (VkCommandBufferBeginInfo) { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(vCmd, &cmdInfo);
	VkBuffer vVertexBuffer = asVkGetBufferFromBuffer(nkVertexBuffer[asVkCurrentFrame]);
	VkBuffer vIndexBuffer = asVkGetBufferFromBuffer(nkIndexBuffer[asVkCurrentFrame]);
	VkDeviceSize vtxOffset = 0;
	vkCmdBindVertexBuffers(vCmd, 0, 1, &vVertexBuffer, &vtxOffset);
	vkCmdBindIndexBuffer(vCmd, vIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

	/*Bind shader pipeline*/
	vkCmdBindPipeline(vCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)nkShaderFx.pipelines[0]);
	vkCmdBindDescriptorSets(vCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkNuklearPipelineLayout, 0, 1, &vkNuklearDefaultFontDescriptorSet, 0, NULL);

	/*Begin Render Pass*/
	VkClearValue clearValues[] = {
		(VkClearValue){
			.color = {0.0f,0.0f,0.0f,1.0f}
		},
		(VkClearValue) {
			.depthStencil = {1.0f, 0}
		}
	};
	VkRenderPassBeginInfo beginInfo = (VkRenderPassBeginInfo){ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.clearValueCount = ASARRAYLEN(clearValues);
	beginInfo.pClearValues = clearValues;
	beginInfo.renderArea = (VkRect2D){ {0,0},{viewWidth, viewHeight} };
	beginInfo.renderPass = asVkGetSimpleDrawingRenderpass();
	beginInfo.framebuffer = asVkGetSimpleDrawingFramebuffer();
	vkCmdBeginRenderPass(vCmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	/*Setup Viewport*/
	VkViewport vkViewport = (VkViewport){ 0 };
	vkViewport.maxDepth = 1.0f;
	vkViewport.width = (float)viewWidth;
	vkViewport.height = (float)viewHeight;
	vkCmdSetViewport(vCmd, 0, 1, &vkViewport);
#endif
	uint32_t indexOffset = 0;
	nk_draw_foreach(nkCmd, &nkContext, &nkCommands)
	{
		if (!nkCmd->elem_count) continue;
#if ASTRENGINE_VK
		/*Set scissors*/
		VkRect2D scissor;
		scissor.offset.x = (int32_t)(nkCmd->clip_rect.x);
		scissor.offset.y = (int32_t)(nkCmd->clip_rect.y);
		scissor.extent.width = (uint32_t)(nkCmd->clip_rect.w);
		scissor.extent.height = (uint32_t)(nkCmd->clip_rect.h);
		if (scissor.extent.width > (uint32_t)viewWidth)
			scissor.extent.width = (uint32_t)viewWidth;
		if (scissor.extent.height > (uint32_t)viewHeight)
			scissor.extent.height = (uint32_t)viewHeight;
		if (scissor.offset.x < 0)
			scissor.offset.x = 0;
		if (scissor.offset.y < 0)
			scissor.offset.y = 0;
		vkCmdSetScissor(vCmd, 0, 1, &scissor);
		/*Draw indexed*/
		vkCmdDrawIndexed(vCmd, nkCmd->elem_count, 1, indexOffset, 0, 0);
#endif
		indexOffset += nkCmd->elem_count;
	}
#if ASTRENGINE_VK
	vkCmdEndRenderPass(vCmd);
	vkEndCommandBuffer(vCmd);
#endif
	nk_clear(&nkContext);
}

ASEXPORT void asNkReset()
{
	nk_clear(&nkContext);
}

ASEXPORT void asNkPushEvent(void *pEvent)
{
	/*Yes... I lifted this straight off of the demos... Not recreating this mess*/
	SDL_Event *evt = (SDL_Event*)pEvent;
	struct nk_context* ctx = &nkContext;
	if (ctx->input.mouse.grab) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
		ctx->input.mouse.grab = 0;
	}
	else if (ctx->input.mouse.ungrab) {
		int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_WarpMouseInWindow(asGetMainWindowPtr(), x, y);
		ctx->input.mouse.ungrab = 0;
	}
	if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
		/* key events */
		int down = evt->type == SDL_KEYDOWN;
		const Uint8* state = SDL_GetKeyboardState(0);
		SDL_Keycode sym = evt->key.keysym.sym;
		if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
			nk_input_key(ctx, NK_KEY_SHIFT, down);
		else if (sym == SDLK_DELETE)
			nk_input_key(ctx, NK_KEY_DEL, down);
		else if (sym == SDLK_RETURN)
			nk_input_key(ctx, NK_KEY_ENTER, down);
		else if (sym == SDLK_TAB)
			nk_input_key(ctx, NK_KEY_TAB, down);
		else if (sym == SDLK_BACKSPACE)
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
		else if (sym == SDLK_HOME) {
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
		}
		else if (sym == SDLK_END) {
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
		}
		else if (sym == SDLK_PAGEDOWN) {
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
		}
		else if (sym == SDLK_PAGEUP) {
			nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
		}
		else if (sym == SDLK_z)
			nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_r)
			nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_c)
			nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_v)
			nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_x)
			nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_b)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_e)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_UP)
			nk_input_key(ctx, NK_KEY_UP, down);
		else if (sym == SDLK_DOWN)
			nk_input_key(ctx, NK_KEY_DOWN, down);
		else if (sym == SDLK_LEFT) {
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else nk_input_key(ctx, NK_KEY_LEFT, down);
		}
		else if (sym == SDLK_RIGHT) {
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else nk_input_key(ctx, NK_KEY_RIGHT, down);
		}
	}
	else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
		/* mouse button */
		int down = evt->type == SDL_MOUSEBUTTONDOWN;
		const int x = evt->button.x, y = evt->button.y;
		if (evt->button.button == SDL_BUTTON_LEFT) {
			if (evt->button.clicks > 1)
				nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
			nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
		}
		else if (evt->button.button == SDL_BUTTON_MIDDLE)
			nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
		else if (evt->button.button == SDL_BUTTON_RIGHT)
			nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
		return;
	}
	else if (evt->type == SDL_MOUSEMOTION) {
		/* mouse motion */
		if (ctx->input.mouse.grabbed) {
			int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
			nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
		}
		else nk_input_motion(ctx, evt->motion.x, evt->motion.y);
		return;
	}
	else if (evt->type == SDL_TEXTINPUT) {
		/* text input */
		nk_glyph glyph;
		memcpy(glyph, evt->text.text, NK_UTF_SIZE);
		nk_input_glyph(ctx, glyph);
		return;
	}
	else if (evt->type == SDL_MOUSEWHEEL) {
		/* mouse wheel */
		nk_input_scroll(ctx, nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
		return;
	}
}

ASEXPORT void asNkBeginInput()
{
	nk_input_begin(&nkContext);
}
ASEXPORT void asNkEndInput()
{
	nk_input_end(&nkContext);
}

ASEXPORT void asShutdownGfxNk()
{
	asFreeShaderFx(&nkShaderFx);
#if ASTRENGINE_VK
	vkDestroyDescriptorPool(asVkDevice, vkNuklearDescriptorPool, AS_VK_MEMCB);
	vkDestroyDescriptorSetLayout(asVkDevice, vkNuklearDescriptorSetLayout, AS_VK_MEMCB);
	vkDestroyPipelineLayout(asVkDevice, vkNuklearPipelineLayout, AS_VK_MEMCB);
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		asVkUnmapMemory(asVkGetAllocFromBuffer(nkVertexBuffer[i]));
		asVkUnmapMemory(asVkGetAllocFromBuffer(nkIndexBuffer[i]));
		asReleaseBuffer(nkVertexBuffer[i]);
		asReleaseBuffer(nkIndexBuffer[i]);
	}
#endif
	asReleaseTexture(nkFontTexture);
}

ASEXPORT void asShutdownNk()
{
	
	nk_buffer_free(&nkCommands);
	nk_font_atlas_clear(&nkFontAtlas);
	asFree(nkMemory);
}
#endif