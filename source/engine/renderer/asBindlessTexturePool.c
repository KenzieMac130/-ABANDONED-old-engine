#include "asBindlessTexturePool.h"

#include "../common/preferences/asPreferences.h"

#if ASTRENGINE_VK
#include "../renderer/vulkan/asVulkanBackend.h"
#include <SDL_vulkan.h>
#endif

asTextureHandle_t missingTextureHndl;
asTextureHandle_t textureHandleAssociations[AS_MAX_POOLED_TEXTURES] = { 0 }; /*Allows Further Remapping for Streaming/Unload*/

#if ASTRENGINE_VK /*Vulkan Globals*/
VkDescriptorSet vTexturePoolDescSet;
VkDescriptorSetLayout vTexturePoolDescLayout;
VkDescriptorPool vTexturePoolDescriptorPool;
#endif

/*Queue of Updates*/
size_t textureUpdateQueueCount = 0;
asBindlessTextureIndex textureUpdateQueue[AS_MAX_POOLED_TEXTURES];

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
	*pTextureIndex = handle._index;

	textureUpdateQueue[textureUpdateQueueCount] = handle._index;
	textureUpdateQueueCount++;
	return AS_SUCCESS;
}

ASEXPORT asResults asTexturePoolRelease(asBindlessTextureIndex textureIndex)
{
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
		if (vkCreateDescriptorSetLayout(asVkDevice, &descriptorSetLayoutInfo, AS_VK_MEMCB, &vTexturePoolDescLayout) != VK_SUCCESS)
			asFatalError("vkCreateDescriptorSetLayout() Failed to create vTexturePoolDescLayout");
	}
	/*Descriptor Pool*/
	{
		VkDescriptorPoolSize fontImagePoolSize = { 0 };
		fontImagePoolSize.descriptorCount = AS_MAX_POOLED_TEXTURES;
		fontImagePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VkDescriptorPoolCreateInfo createInfo = (VkDescriptorPoolCreateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &fontImagePoolSize;
		if (vkCreateDescriptorPool(asVkDevice, &createInfo, AS_VK_MEMCB, &vTexturePoolDescriptorPool) != VK_SUCCESS)
			asFatalError("vkCreateDescriptorPool() Failed to create vTexturePoolDescriptorPool");
	}
	/*Descriptor Set*/
	{
		VkDescriptorSetAllocateInfo descSetAllocInfo = (VkDescriptorSetAllocateInfo){ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descSetAllocInfo.descriptorPool = vTexturePoolDescriptorPool;
		descSetAllocInfo.descriptorSetCount = 1;
		descSetAllocInfo.pSetLayouts = &vTexturePoolDescLayout;
		if (vkAllocateDescriptorSets(asVkDevice, &descSetAllocInfo, &vTexturePoolDescSet) != VK_SUCCESS)
			asFatalError("vkAllocateDescriptorSets() Failed to allocate vTexturePoolDescSet");
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
			VkWriteDescriptorSet descSetWrite = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descSetWrite.dstSet = vTexturePoolDescSet;
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
#endif
	return AS_SUCCESS;
}

ASEXPORT asResults asTexturePoolUpdate()
{
	if (textureUpdateQueueCount <= 0) { return AS_SUCCESS; }

	/*Update Descriptors*/
	for (size_t i = 0; i < textureUpdateQueueCount; i++)
	{
		VkWriteDescriptorSet descSetWrite = (VkWriteDescriptorSet){ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descSetWrite.dstSet = vTexturePoolDescSet;
		descSetWrite.dstBinding = AS_BINDING_TEXTURE_POOL;
		descSetWrite.descriptorCount = 1;
		descSetWrite.dstArrayElement = textureUpdateQueue[i];
		descSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descSetWrite.pImageInfo = &(VkDescriptorImageInfo) {
			.imageView = asVkGetViewFromTexture(textureHandleAssociations[textureUpdateQueue[i]]),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.sampler = *asVkGetSimpleSamplerPtr(true)
		};
		vkUpdateDescriptorSets(asVkDevice, 1, &descSetWrite, 0, NULL);
	}
	textureUpdateQueueCount = 0;
	return AS_SUCCESS;
}

ASEXPORT asResults asShutdownTexturePool()
{
#if ASTRENGINE_VK
	vkDestroyDescriptorPool(asVkDevice, vTexturePoolDescriptorPool, AS_VK_MEMCB);
#endif
	return AS_SUCCESS;
}

ASEXPORT void asVkGetTexturePoolDescSetLayout(void* pDest)
{
	memcpy(pDest, &vTexturePoolDescLayout, sizeof(vTexturePoolDescLayout));
}

ASEXPORT asResults asTexturePoolBindCmd(asGfxAPIs apiValidate, void* pCmdBuff, void* pLayout)
{
#if ASTRENGINE_VK
	ASASSERT(apiValidate == AS_GFXAPI_VULKAN);
	VkCommandBuffer cmdBuffer = *(VkCommandBuffer*)pCmdBuff;
	VkPipelineLayout pipelineLayout = *(VkPipelineLayout*)pLayout;
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, AS_DESCSET_TEXTURE_POOL, 1, &vTexturePoolDescSet, 0, NULL);
#endif
	return AS_SUCCESS;
}