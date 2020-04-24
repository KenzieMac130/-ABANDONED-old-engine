#include "asDearImGuiImplimentation.h"

#include "../common/preferences/asPreferences.h"

#if ASTRENGINE_DEARIMGUI

#include "SDL_events.h"
#include "SDL_clipboard.h"
#if _WIN32
#include "SDL_syswm.h" /*Windows IME*/
#endif

#if ASTRENGINE_VK
#include "../renderer/vulkan/asVulkanBackend.h"
#include <SDL_vulkan.h>
#endif

#include "../renderer/asBindlessTexturePool.h"

#define AS_IMGUI_MAX_VERTS 65536
#define AS_IMGUI_MAX_INDICES 65536
ImGuiContext* pImGuiContext;
ImGuiIO* pImGuiIo;

asTextureHandle_t imGuiFontTexture;
asBindlessTextureIndex imGuiFontTextureIndex;

asBufferHandle_t imGuiVertexBuffer[AS_MAX_INFLIGHT];
asBufferHandle_t imGuiIndexBuffer[AS_MAX_INFLIGHT];

asShaderFx imGuiShaderFx;

#define AS_BINDING_GUI_TEXTURE 300

#if ASTRENGINE_VK
VkPipelineLayout imGuiPipelineLayout;
VkRenderPass imGuiRenderPass;
VkFramebuffer imGuiFramebuffers[1];
#endif

typedef struct {
	float scale[2];
	float offset[2];
	uint32_t textureIdx;
} imGuiPushData;

#if ASTRENGINE_VK
ASEXPORT asResults _asFillGfxPipeline_DearImgui(
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
		AS_VK_INIT_HELPER_VERTEX_BINDING(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX)
	};
	VkVertexInputAttributeDescription vertexAttributes[] = { 
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),
		AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col))
	};
	pGraphicsPipelineDesc->pVertexInputState = &AS_VK_INIT_HELPER_PIPE_VERTEX_STATE(
		ASARRAYLEN(vertexBindings), vertexBindings,
		ASARRAYLEN(vertexAttributes), vertexAttributes);

	pGraphicsPipelineDesc->renderPass = imGuiRenderPass;
	pGraphicsPipelineDesc->subpass = 0;
	pGraphicsPipelineDesc->layout = imGuiPipelineLayout;

	if(vkCreateGraphicsPipelines(asVkDevice, VK_NULL_HANDLE, 1, pGraphicsPipelineDesc, AS_VK_MEMCB, (VkPipeline*)pPipelineOut) != VK_SUCCESS)
		asFatalError("vkCreateGraphicsPipelines() Failed to create imGuiPipeline");
	return AS_SUCCESS;
}

/*Vulkan allows mappings to stay persistant*/
void* vVertexBufferBindings[AS_MAX_INFLIGHT];
void* vIndexBufferBindings[AS_MAX_INFLIGHT];
#endif

void _imguiGetRenderDim(int vp)
{
	int x, y;
	asGetRenderDimensions(vp, true, &x, &y);
	pImGuiIo->DisplaySize = (ImVec2){ (float)x, (float)y };
}

/*Clipboard*/
void _imguiSetClipboard(void* userData, const char* text)
{
	SDL_SetClipboardText(text);
}

char* _clipboardContent = NULL;
const char* _imguiGetClipboard(void* userData)
{
	if (_clipboardContent)
		SDL_free(_clipboardContent);
	_clipboardContent = SDL_GetClipboardText();
	return _clipboardContent;
}

static int32_t imguiDrawTestWindow = 0;
static int32_t imguiDrawStatistics = 0;
static int32_t imguiDrawStyle = 0;

/*Cursor*/
static SDL_Cursor* _mouseCursors[ImGuiMouseCursor_COUNT];

ASEXPORT void asInitImGui()
{
	asDebugLog("Creating ImGui Backend...");

	/*Settings*/
	asPreferencesRegisterOpenSection(asGetGlobalPrefs(), "imgui");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "demoWindowVisible", &imguiDrawTestWindow, 0, 1, false, NULL, NULL,
		"Show ImGui Demo Window");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "statisticsVisible", &imguiDrawStatistics, 0, 1, false, NULL, NULL,
		"Show ImGui Statistics");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "styleEditorVisible", &imguiDrawStyle, 0, 1, false, NULL, NULL,
		"Show ImGui Style Editor");
	asPreferencesLoadSection(asGetGlobalPrefs(), "imgui");

	unsigned char* img;
	int w = 0, h = 0;
	/*Setup Imgui*/
	{
		pImGuiContext = igCreateContext(NULL);
		igStyleColorsDark(NULL);
		pImGuiIo = igGetIO();
		ImFontAtlas_GetTexDataAsRGBA32(pImGuiIo->Fonts, &img, &w, &h, NULL);
		_imguiGetRenderDim(0);
	}
	/*Input Mapping*/
	{
		pImGuiIo->BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		pImGuiIo->BackendPlatformName = "astrengine";

		pImGuiIo->KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
		pImGuiIo->KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
		pImGuiIo->KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
		pImGuiIo->KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
		pImGuiIo->KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
		pImGuiIo->KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
		pImGuiIo->KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
		pImGuiIo->KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
		pImGuiIo->KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
		pImGuiIo->KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
		pImGuiIo->KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
		pImGuiIo->KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
		pImGuiIo->KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
		pImGuiIo->KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
		pImGuiIo->KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
		pImGuiIo->KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
		pImGuiIo->KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
		pImGuiIo->KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
		pImGuiIo->KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
		pImGuiIo->KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
		pImGuiIo->KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
		pImGuiIo->KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

		pImGuiIo->ClipboardUserData = NULL;
		pImGuiIo->SetClipboardTextFn = _imguiSetClipboard;
		pImGuiIo->GetClipboardTextFn = _imguiGetClipboard;

		_mouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		_mouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
		_mouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		_mouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
		_mouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
		_mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
		_mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
		_mouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		_mouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

		pImGuiIo->IniFilename = NULL;
		pImGuiIo->LogFilename = NULL;

#ifdef _WIN32
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(asGetMainWindowPtr(), &wmInfo);
		pImGuiIo->ImeWindowHandle = wmInfo.info.win.window;
#endif

	}
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
		desc.pDebugLabel = "DearImGuiFont";
		imGuiFontTexture = asCreateTexture(&desc);
		asTexturePoolAddFromHandle(imGuiFontTexture, &imGuiFontTextureIndex);
		pImGuiIo->Fonts->TexID = imGuiFontTextureIndex;
	}
	/*Create vertex buffer*/
	{
		asBufferDesc_t desc = asBufferDesc_Init();
		desc.bufferSize = AS_IMGUI_MAX_VERTS * sizeof(ImDrawVert);
		desc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
		desc.usageFlags = AS_BUFFERUSAGE_VERTEX;
		desc.initialContentsBufferSize = desc.bufferSize;
		desc.pDebugLabel = "DearImGuiVerts";
		for(int i = 0; i < AS_MAX_INFLIGHT; i++)
			imGuiVertexBuffer[i] = asCreateBuffer(&desc);
	}
	/*Create index buffer*/
	{
		asBufferDesc_t desc = asBufferDesc_Init();
		desc.bufferSize = AS_IMGUI_MAX_INDICES * sizeof(uint16_t);
		desc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
		desc.usageFlags = AS_BUFFERUSAGE_INDEX;
		desc.initialContentsBufferSize = desc.bufferSize;
		desc.pDebugLabel = "DearImGuiIndices";
		for (int i = 0; i < AS_MAX_INFLIGHT; i++)
			imGuiIndexBuffer[i] = asCreateBuffer(&desc);
	}
#if ASTRENGINE_VK
	/*Map persistent memory*/
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		asVkAllocation_t vertAlloc = asVkGetAllocFromBuffer(imGuiVertexBuffer[i]);
		asVkAllocation_t idxAlloc = asVkGetAllocFromBuffer(imGuiIndexBuffer[i]);
		asVkMapMemory(vertAlloc, 0, vertAlloc.size, &vVertexBufferBindings[i]);
		asVkMapMemory(idxAlloc, 0, idxAlloc.size, &vIndexBufferBindings[i]);
	}
	/*Setup Pipeline Layout*/
	{
		VkDescriptorSetLayout descSetLayouts[1] = { 0 };
		asVkGetTexturePoolDescSetLayout(&descSetLayouts[0]);

		VkPipelineLayoutCreateInfo createInfo = (VkPipelineLayoutCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = ASARRAYLEN(descSetLayouts);
		createInfo.pSetLayouts = descSetLayouts;
		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges = &(VkPushConstantRange) { VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(imGuiPushData) };
		if (vkCreatePipelineLayout(asVkDevice, &createInfo, AS_VK_MEMCB, &imGuiPipelineLayout) != VK_SUCCESS)
			asFatalError("vkCreatePipelineLayout() Failed to create imGuiPipelineLayout");
	}
	/*Renderpass*/
	{
		VkAttachmentDescription attachments[] = {
			(VkAttachmentDescription) { /*Color Buffer*/
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

		if (vkCreateRenderPass(asVkDevice, &createInfo, AS_VK_MEMCB, &imGuiRenderPass) != VK_SUCCESS)
			asFatalError("vkCreateRenderPass() Failed to create pScreen->simpleRenderPass");
	}
	/*Framebuffer (For Each Screen)*/
	{
		const int screenIdx = 0;
		asVkScreenResources* pScreen = asVkGetScreenResourcesPtr(screenIdx);
		VkImageView attachments[] = {
			asVkGetViewFromTexture(pScreen->compositeTexture),
			asVkGetViewFromTexture(pScreen->depthTexture)
		};
		VkFramebufferCreateInfo createInfo = (VkFramebufferCreateInfo){ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = imGuiRenderPass;
		createInfo.attachmentCount = ASARRAYLEN(attachments);
		createInfo.pAttachments = attachments;
		createInfo.width = pScreen->extents.width;
		createInfo.height = pScreen->extents.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(asVkDevice, &createInfo, AS_VK_MEMCB, &imGuiFramebuffers[screenIdx]) != VK_SUCCESS)
			asFatalError("vkCreateFramebuffer() Failed to create pScreen->simpleFrameBuffer");
	}
#endif
	/*Load shader*/
	{
		unsigned char* fileData;
		size_t fileSize;
		asResourceLoader_t resourceLoader;
		asResourceFileID_t resourceFileId;
		asResourceLoader_OpenByPath(&resourceLoader, &resourceFileId, "shaders/core/DearImGui_FX.asfx", 30);
		fileSize = asResourceLoader_GetContentSize(&resourceLoader);
		fileData = asMalloc(fileSize);
		asResourceLoader_ReadAll(&resourceLoader, fileSize, fileData);
		asResourceLoader_Close(&resourceLoader);

		asBinReader shaderBin;
		asBinReaderOpenMemory(&shaderBin, "ASFX", fileData, fileSize);
		if (asCreateShaderFx(&shaderBin, &imGuiShaderFx, AS_QUALITY_HIGH) != AS_SUCCESS)
			asFatalError("Could not load from shader database \"shaders/core/DearImGui_FX.asfx\"");
		asFree(fileData);
	}

	igNewFrame();
}

ASEXPORT void asImGuiDraw(int32_t viewport)
{
	/*Drawing*/
	int viewWidth, viewHeight;
	asGetRenderDimensions(viewport, true, &viewWidth, &viewHeight);
	_imguiGetRenderDim(viewport);

	/*Render and Get Draw Data*/
	igRender();
	ImDrawData* pDrawData = igGetDrawData();
	/*Upload vertices*/
	{
		ImDrawVert* pVertDest = vVertexBufferBindings[asVkCurrentFrame];
		ImDrawIdx* pIdxDest = vIndexBufferBindings[asVkCurrentFrame];
		size_t vtxCount = 0;
		size_t idxCount = 0;
		for (int i = 0; i < pDrawData->CmdListsCount; i++)
		{
			vtxCount += pDrawData->CmdLists[i]->VtxBuffer.Size;
			idxCount += pDrawData->CmdLists[i]->IdxBuffer.Size;
			if (vtxCount > AS_IMGUI_MAX_VERTS) { asDebugWarning("ImGui Vertex Overflow!!!"); continue; }
			if (idxCount > AS_IMGUI_MAX_INDICES) { asDebugWarning("ImGui Index Overflow!!!"); continue; }
			memcpy(pVertDest, pDrawData->CmdLists[i]->VtxBuffer.Data, pDrawData->CmdLists[i]->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(pIdxDest, pDrawData->CmdLists[i]->IdxBuffer.Data, pDrawData->CmdLists[i]->IdxBuffer.Size * sizeof(ImDrawIdx));
			pIdxDest += pDrawData->CmdLists[i]->IdxBuffer.Size;
			pVertDest += pDrawData->CmdLists[i]->VtxBuffer.Size;
		}

#if ASTRENGINE_VK
		asVkAllocation_t vertAlloc = asVkGetAllocFromBuffer(imGuiVertexBuffer[asVkCurrentFrame]);
		asVkAllocation_t idxAlloc = asVkGetAllocFromBuffer(imGuiIndexBuffer[asVkCurrentFrame]);
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
	VkBuffer vVertexBuffer = asVkGetBufferFromBuffer(imGuiVertexBuffer[asVkCurrentFrame]);
	VkBuffer vIndexBuffer = asVkGetBufferFromBuffer(imGuiIndexBuffer[asVkCurrentFrame]);
	VkDeviceSize vtxOffset = 0;
	vkCmdBindVertexBuffers(vCmd, 0, 1, &vVertexBuffer, &vtxOffset);
	vkCmdBindIndexBuffer(vCmd, vIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

	/*Bind shader pipeline*/
	vkCmdBindPipeline(vCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)imGuiShaderFx.pipelines[0]);
	asTexturePoolBindCmd(AS_GFXAPI_VULKAN, &vCmd, &imGuiPipelineLayout);

	/*Begin Render Pass*/
	VkClearValue clearValues[] = {
		(VkClearValue){
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
	beginInfo.renderPass = imGuiRenderPass;
	beginInfo.framebuffer = imGuiFramebuffers[viewport];
	vkCmdBeginRenderPass(vCmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	/*Setup Viewport*/
	VkViewport vkViewport = (VkViewport){ 0 };
	vkViewport.maxDepth = 1.0f;
	vkViewport.width = (float)viewWidth;
	vkViewport.height = (float)viewHeight;
	vkCmdSetViewport(vCmd, 0, 1, &vkViewport);
#endif
	uint32_t listIdxOffset = 0;
	uint32_t listVtxOffset = 0;
	for (int i = 0; i < pDrawData->CmdListsCount; i++)
	{
		if (pDrawData->CmdListsCount > 1) 
		{ 
			bool doshit = true;
		}
		const ImDrawList* imList = pDrawData->CmdLists[i];
		for (int c = 0; c < imList->CmdBuffer.Size; c++)
		{
			const ImDrawCmd* imCmd = &imList->CmdBuffer.Data[c];
			if (!imCmd->ElemCount) continue;
#if ASTRENGINE_VK

			/*Vulkan Clip Space Transforms (from imgui examples)*/
			ImVec2 clip_off = pDrawData->DisplayPos;
			ImVec2 clip_scale = pDrawData->FramebufferScale;
			ImVec4 clip_rect;
			clip_rect.x = (imCmd->ClipRect.x - clip_off.x) * clip_scale.x;
			clip_rect.y = (imCmd->ClipRect.y - clip_off.y) * clip_scale.y;
			clip_rect.z = (imCmd->ClipRect.z - clip_off.x) * clip_scale.x;
			clip_rect.w = (imCmd->ClipRect.w - clip_off.y) * clip_scale.y;
			if (!(clip_rect.x < viewWidth && clip_rect.y < viewHeight && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)) { continue; }

			/*Set scissors*/
			VkRect2D scissor;
			scissor.offset.x = (int32_t)(clip_rect.x);
			scissor.offset.y = (int32_t)(clip_rect.y);
			scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
			scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
			if (scissor.extent.width > (uint32_t)viewWidth)
				scissor.extent.width = (uint32_t)viewWidth;
			if (scissor.extent.height > (uint32_t)viewHeight)
				scissor.extent.height = (uint32_t)viewHeight;
			if (scissor.offset.x < 0)
				scissor.offset.x = 0;
			if (scissor.offset.y < 0)
				scissor.offset.y = 0;
			vkCmdSetScissor(vCmd, 0, 1, &scissor);
			/*Inject Data*/
			float scale[2];
			scale[0] = 2.0f / pDrawData->DisplaySize.x;
			scale[1] = 2.0f / pDrawData->DisplaySize.y;
			float translate[2];
			translate[0] = -1.0f - pDrawData->DisplayPos.x * scale[0];
			translate[1] = -1.0f - pDrawData->DisplayPos.y * scale[1];
			imGuiPushData pushData = { {scale[0],scale[1]}, {translate[0], translate[1]}, (uint32_t)(size_t)imCmd->TextureId };
			vkCmdPushConstants(vCmd, imGuiPipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(imGuiPushData), &pushData);
			/*Draw indexed*/
			vkCmdDrawIndexed(vCmd, imCmd->ElemCount, 1, imCmd->IdxOffset + listIdxOffset, imCmd->VtxOffset + listVtxOffset, 0);
#endif
		}
		listIdxOffset += imList->IdxBuffer.Size;
		listVtxOffset += imList->VtxBuffer.Size;
	}
#if ASTRENGINE_VK
	vkCmdEndRenderPass(vCmd);
	vkEndCommandBuffer(vCmd);
#endif
}

ASEXPORT void asImGuiNewFrame(float time)
{
	igNewFrame();
	pImGuiIo->DeltaTime = time;

	/*Test Windows*/
	if (imguiDrawTestWindow) { igShowDemoWindow(NULL); }
	if (imguiDrawStatistics) { igShowMetricsWindow(NULL); }
	if (imguiDrawStyle) { igShowStyleEditor(NULL); }
}

ASEXPORT void asImGuiEndFrame()
{
	igEndFrame();
}

ASEXPORT void asImGuiReset()
{
	igRender();
}

bool _mousePressed[3];
ASEXPORT void asImGuiPushEvent(void *evt)
{
	SDL_Event * pEvent = (SDL_Event*)evt;
	/*DearImgui input handling
	https://github.com/ocornut/imgui/blob/58b3e02b95b4c7c5bb9128a28c6d55546501bf93/examples/imgui_impl_sdl.cpp */
	switch (pEvent->type)
	{
		case SDL_MOUSEWHEEL:
		{
			if (pEvent->wheel.x > 0) pImGuiIo->MouseWheelH += 1;
			if (pEvent->wheel.x < 0) pImGuiIo->MouseWheelH -= 1;
			if (pEvent->wheel.y > 0) pImGuiIo->MouseWheel += 1;
			if (pEvent->wheel.y < 0) pImGuiIo->MouseWheel -= 1;
			return;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			if (pEvent->button.button == SDL_BUTTON_LEFT) _mousePressed[0] = true;
			if (pEvent->button.button == SDL_BUTTON_RIGHT) _mousePressed[1] = true;
			if (pEvent->button.button == SDL_BUTTON_MIDDLE) _mousePressed[2] = true;
			return;
		}
		case SDL_TEXTINPUT:
		{
			ImGuiIO_AddInputCharactersUTF8(pImGuiIo, pEvent->text.text);
			return;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			int key = pEvent->key.keysym.scancode;
			ASASSERT(key >= 0 && key < ASARRAYLEN(pImGuiIo->KeysDown));
			pImGuiIo->KeysDown[key] = (pEvent->type == SDL_KEYDOWN);
			pImGuiIo->KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			pImGuiIo->KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
			pImGuiIo->KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
	#ifdef _WIN32
			pImGuiIo->KeySuper = false;
	#else
			pImGuiIo->KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
	#endif
			return;
		}
	}
}

ASEXPORT void asImGuiPumpInput()
{
	/*Mouse Cursor*/
	{
		if (pImGuiIo->ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
			return;

		ImGuiMouseCursor imgui_cursor = igGetMouseCursor();
		if (pImGuiIo->MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
		{
			SDL_ShowCursor(SDL_FALSE);
		}
		else
		{
			SDL_SetCursor(_mouseCursors[imgui_cursor] ? _mouseCursors[imgui_cursor] : _mouseCursors[ImGuiMouseCursor_Arrow]);
			SDL_ShowCursor(SDL_TRUE);
		}
	}
	/*Mouse Input (from https://github.com/ocornut/imgui/blob/master/examples/imgui_impl_sdl.cpp )*/
	{
		SDL_Window* window = asGetMainWindowPtr();
		if (pImGuiIo->WantSetMousePos)
			SDL_WarpMouseInWindow(window, (int)pImGuiIo->MousePos.x, (int)pImGuiIo->MousePos.y);
		else
			pImGuiIo->MousePos = (ImVec2){ -FLT_MAX, -FLT_MAX };

		int mx = 0, my = 0;
		Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
		pImGuiIo->MouseDown[0] = _mousePressed[0] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
		pImGuiIo->MouseDown[1] = _mousePressed[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
		pImGuiIo->MouseDown[2] = _mousePressed[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
		memset(_mousePressed, 0, sizeof(bool) * ASARRAYLEN(_mousePressed));
		
		/*The original has handling for global mouse states here... astrengine targets desktop only*/

		if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)
			pImGuiIo->MousePos = (ImVec2){(float)mx, (float)my};
	}
}

ASEXPORT void asShutdownGfxImGui()
{
	asFreeShaderFx(&imGuiShaderFx);
#if ASTRENGINE_VK
	vkDestroyPipelineLayout(asVkDevice, imGuiPipelineLayout, AS_VK_MEMCB);
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		asVkUnmapMemory(asVkGetAllocFromBuffer(imGuiVertexBuffer[i]));
		asVkUnmapMemory(asVkGetAllocFromBuffer(imGuiIndexBuffer[i]));
		asReleaseBuffer(imGuiVertexBuffer[i]);
		asReleaseBuffer(imGuiIndexBuffer[i]);
	}
	vkDestroyRenderPass(asVkDevice, imGuiRenderPass, AS_VK_MEMCB);
	vkDestroyFramebuffer(asVkDevice, imGuiFramebuffers[0], AS_VK_MEMCB);
#endif
	asTexturePoolRelease(imGuiFontTextureIndex);
	asReleaseTexture(imGuiFontTexture);
}

ASEXPORT void asShutdownImGui()
{
	igDestroyContext(pImGuiContext);
	
	if (_clipboardContent)
		SDL_free(_clipboardContent);
	for (int i = 0; i < ASARRAYLEN(_mouseCursors); i++) {
		SDL_FreeCursor(_mouseCursors[i]);
	}
}
#endif