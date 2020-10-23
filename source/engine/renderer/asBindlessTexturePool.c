#include "asBindlessTexturePool.h"

#include "../common/preferences/asPreferences.h"

#if ASTRENGINE_VK
#include "../renderer/vulkan/asVulkanBackend.h"
#include <SDL_vulkan.h>
#endif

asTextureHandle_t missingTextureHndl;
asTextureHandle_t textureHandleAssociations[AS_MAX_POOLED_TEXTURES] = { 0 }; /*Allows Further Remapping for Streaming/Unload*/

#if ASTRENGINE_VK /*Vulkan Globals*/
int32_t texturePoolFrame = 0;
VkDescriptorSet vTexturePoolDescSets[AS_MAX_INFLIGHT];
VkDescriptorSetLayout vTexturePoolDescLayout;
VkDescriptorPool vTexturePoolDescriptorPool;
#endif

/*Queue of Updates*/
#define TEXTURE_UPDATE_QUEUE_COUNT (AS_MAX_INFLIGHT + 1)
int32_t textureUpdateQueueCurrent = 0;
size_t textureUpdateQueueCounts[TEXTURE_UPDATE_QUEUE_COUNT] = { 0 };
asBindlessTextureIndex textureUpdateQueues[TEXTURE_UPDATE_QUEUE_COUNT][AS_MAX_POOLED_TEXTURES];

/*Debug Window*/
int32_t showTexturePoolDebugWindow = 0;

#if ASTRENGINE_DEARIMGUI
#include "../cimgui/asDearImGuiImplimentation.h"
#endif

void _asTexturePoolDebug()
{
	if (!showTexturePoolDebugWindow) { return; }
#if ASTRENGINE_DEARIMGUI
	if (igBegin("Texture Pool Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static asBindlessTextureIndex idx;
		static float squash = 1.0f;
		static float stretch = 1.0f;
		igDragInt("Texture Idx", &idx, 0.1f, 0, AS_MAX_POOLED_TEXTURES, NULL);
		igDragFloat("W", &squash, 0.1f, 0.0, 5.0, NULL, 1.0);
		igDragFloat("H", &stretch, 0.1f, 0.0, 5.0, NULL, 1.0);
		igImage((ImTextureID)idx, (ImVec2) { 256.0f * squash, 256.0f * stretch }, (ImVec2) { 0, 0 }, (ImVec2) { 1, 1 }, (ImVec4) { 1, 1, 1, 1 }, (ImVec4) { 1, 1, 1, 1 });
	}
	igEnd();
#endif
}

ASEXPORT asResults asTexturePoolAddFromHandle(const asTextureHandle_t handle, asBindlessTextureIndex* pTextureIndex)
{
	if (handle._index >= AS_MAX_POOLED_TEXTURES) { return AS_FAILURE_OUT_OF_BOUNDS; }
	textureHandleAssociations[handle._index] = handle;
	if (pTextureIndex) { *pTextureIndex = handle._index; }

	textureUpdateQueues[textureUpdateQueueCurrent][textureUpdateQueueCounts[textureUpdateQueueCurrent]] = handle._index;
	textureUpdateQueueCounts[textureUpdateQueueCurrent]++;
	return AS_SUCCESS;
}

ASEXPORT asResults asTexturePoolRelease(asBindlessTextureIndex textureIndex)
{
	textureHandleAssociations[textureIndex] = missingTextureHndl;
	textureUpdateQueues[textureUpdateQueueCurrent][textureUpdateQueueCounts[textureUpdateQueueCurrent]] = missingTextureHndl._index;
	textureUpdateQueueCounts[textureUpdateQueueCurrent]++;
	return AS_SUCCESS;
}

ASEXPORT asResults asInitTexturePool()
{
	asPreferencesRegisterOpenSection(asGetGlobalPrefs(), "gfx");
	asPreferencesRegisterParamInt32(asGetGlobalPrefs(), "debugActiveTextureViewer", &showTexturePoolDebugWindow, 0, 1, false, NULL, NULL, "Show Texture Pool Dump");

	/*Missing Texture*/
#if ASTRENGINE_VK
	/*Descriptor Set Layout*/
	{
		VkDescriptorSetLayoutBinding descriptorBindings[] = {

			(VkDescriptorSetLayoutBinding) {
				.binding = AS_BINDING_TEXTURE_POOL,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = AS_MAX_POOLED_TEXTURES,
				.stageFlags = VK_SHADER_STAGE_ALL,
			}
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetLayoutInfo.bindingCount = ASARRAYLEN(descriptorBindings);
		descriptorSetLayoutInfo.pBindings = descriptorBindings;
		AS_VK_CHECK(vkCreateDescriptorSetLayout(asVkDevice, &descriptorSetLayoutInfo, AS_VK_MEMCB, &vTexturePoolDescLayout),
			"vkCreateDescriptorSetLayout() Failed to create vTexturePoolDescLayout");
	}
	/*Descriptor Pool*/
	{
		VkDescriptorPoolSize fontImagePoolSize = { 0 };
		fontImagePoolSize.descriptorCount = AS_MAX_POOLED_TEXTURES * AS_MAX_INFLIGHT;
		fontImagePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VkDescriptorPoolCreateInfo createInfo = (VkDescriptorPoolCreateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.maxSets = 2;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &fontImagePoolSize;
		AS_VK_CHECK(vkCreateDescriptorPool(asVkDevice, &createInfo, AS_VK_MEMCB, &vTexturePoolDescriptorPool),
			"vkCreateDescriptorPool() Failed to create vTexturePoolDescriptorPool");
	}
	/*Descriptor Set*/
	{
		VkDescriptorSetLayout layouts[AS_MAX_INFLIGHT];
		for (int i = 0; i < AS_MAX_INFLIGHT; i++) { layouts[i] = vTexturePoolDescLayout; }
		VkDescriptorSetAllocateInfo descSetAllocInfo = (VkDescriptorSetAllocateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descSetAllocInfo.descriptorPool = vTexturePoolDescriptorPool;
		descSetAllocInfo.descriptorSetCount = AS_MAX_INFLIGHT;
		descSetAllocInfo.pSetLayouts = layouts;
		AS_VK_CHECK(vkAllocateDescriptorSets(asVkDevice, &descSetAllocInfo, vTexturePoolDescSets),
			"vkAllocateDescriptorSets() Failed to allocate vTexturePoolDescSets");
	}
	/*Set all as Blank Texture*/
	{
		uint32_t tex[4][4];
		memset(tex, 255, 4 * 4 * 4);
		asTextureDesc_t desc = asTextureDesc_Init();
		desc.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
		desc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		desc.format = AS_COLORFORMAT_RGBA8_UNORM;
		desc.width = 4;
		desc.height = 4;
		desc.initialContentsBufferSize = (size_t)(4 * 4 * 4);
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
		desc.pInitialContentsBuffer = tex;
		desc.pDebugLabel = "MissingTexture";
		missingTextureHndl = asCreateTexture(&desc);

		for (size_t i = 0; i < AS_MAX_POOLED_TEXTURES; i++)
		{
			for (int f = 0; f < AS_MAX_INFLIGHT; f++)
			{
				VkWriteDescriptorSet descSetWrite = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				descSetWrite.dstSet = vTexturePoolDescSets[f];
				descSetWrite.dstBinding = AS_BINDING_TEXTURE_POOL;
				descSetWrite.descriptorCount = 1;
				descSetWrite.dstArrayElement = i;
				descSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descSetWrite.pImageInfo = &(VkDescriptorImageInfo) {
					.imageView = asVkGetViewFromTexture(missingTextureHndl),
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.sampler = *asVkGetSimpleSamplerPtr(true)
				};
				vkUpdateDescriptorSets(asVkDevice, 1, &descSetWrite, 0, NULL);
			}
		}
	}
#endif
	return AS_SUCCESS;
}

ASEXPORT asResults asTexturePoolUpdate()
{
	/*Update Descriptors*/
	textureUpdateQueueCurrent = texturePoolFrame;
	texturePoolFrame = asVkCurrentFrame;
	for (int f = 0; f < AS_MAX_INFLIGHT; f++)
	{
		int queueIdx = abs((f + textureUpdateQueueCurrent - TEXTURE_UPDATE_QUEUE_COUNT) % AS_MAX_INFLIGHT);
		for (size_t i = 0; i < textureUpdateQueueCounts[queueIdx]; i++)
		{
			VkWriteDescriptorSet descSetWrite = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descSetWrite.dstSet = vTexturePoolDescSets[texturePoolFrame];
			descSetWrite.dstBinding = AS_BINDING_TEXTURE_POOL;
			descSetWrite.descriptorCount = 1;
			descSetWrite.dstArrayElement = textureUpdateQueues[queueIdx][i];
			descSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descSetWrite.pImageInfo = &(VkDescriptorImageInfo) {
				.imageView = asVkGetViewFromTexture(textureHandleAssociations[textureUpdateQueues[queueIdx][i]]),
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.sampler = *asVkGetSimpleSamplerPtr(true)
			};
			vkUpdateDescriptorSets(asVkDevice, 1, &descSetWrite, 0, NULL);
		}
		if (f == TEXTURE_UPDATE_QUEUE_COUNT - 1) { textureUpdateQueueCounts[queueIdx] = 0; }
	}
	return AS_SUCCESS;
}

ASEXPORT asResults asShutdownTexturePool()
{
#if ASTRENGINE_VK
	vkDestroyDescriptorSetLayout(asVkDevice, vTexturePoolDescLayout, AS_VK_MEMCB);
	vkDestroyDescriptorPool(asVkDevice, vTexturePoolDescriptorPool, AS_VK_MEMCB);
#endif
	return AS_SUCCESS;
}

ASEXPORT void asVkGetTexturePoolDescSetLayout(void* pDest)
{
	memcpy(pDest, &vTexturePoolDescLayout, sizeof(vTexturePoolDescLayout));
}

ASEXPORT asResults asTexturePoolBindCmd(asGfxAPIs apiValidate, void* pCmdBuff, void* pLayout, int frame)
{
#if ASTRENGINE_VK
	ASASSERT(apiValidate == AS_GFXAPI_VULKAN);
	VkCommandBuffer cmdBuffer = *(VkCommandBuffer*)pCmdBuff;
	VkPipelineLayout pipelineLayout = *(VkPipelineLayout*)pLayout;
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout, AS_DESCSET_TEXTURE_POOL, 1, &vTexturePoolDescSets[frame], 0, NULL);
#endif
	return AS_SUCCESS;
}