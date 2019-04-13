#include "include/lowlevel/asVulkanBackend.h"
#if ASTRENGINE_VK
#include <SDL_vulkan.h>
#include "include/asRendererCore.h"

struct asVkAllocation_t {
	VkDeviceMemory memHandle;
	uint32_t memType;
	VkDeviceSize size;
	VkDeviceSize offset;
};

struct vScreenResources_t
{
	SDL_Window *pWindow;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkSurfaceCapabilitiesKHR caps;
	VkSurfaceFormatKHR surfFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extents;

	uint32_t imageCount;
	VkImage *pSwapImages;
};
struct vScreenResources_t vMainScreen;

VkInstance asVkInstance = VK_NULL_HANDLE;
VkPhysicalDevice asVkPhysicalDevice = VK_NULL_HANDLE;

VkPhysicalDeviceProperties asVkDeviceProperties;
VkPhysicalDeviceFeatures asVkDeviceFeatures;
VkPhysicalDeviceMemoryProperties asVkDeviceMemProps;

VkDevice asVkDevice;
VkQueue asVkQueue_GFX;
VkQueue asVkQueue_Present;

VkCommandPool asVkMainCommandPool;

typedef struct
{
	uint32_t graphicsIdx;
	uint32_t presentIdx;
} vQueueFamilyIndices_t;
vQueueFamilyIndices_t asVkQueueFamilyIndices;

bool vIsQueueFamilyComplete(vQueueFamilyIndices_t indices)
{
	return (indices.graphicsIdx != UINT32_MAX && indices.presentIdx != UINT32_MAX);
}

const char* deviceReqExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#if AS_VK_VALIDATION
const char* validationLayers[] = {
	"VK_LAYER_LUNARG_standard_validation"
};

bool vValidationLayersAvailible()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	VkLayerProperties *layerProps = (VkLayerProperties*)asAlloc_LinearMalloc(sizeof(VkLayerProperties) * layerCount, 0);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProps);
	for (uint32_t i = 0; i < layerCount; i++)
		asDebugLog("VK Layer: \"%s\" found\n", layerProps[i].layerName);
	bool found;
	for (uint32_t i = 0; i < ASARRAYSIZE(validationLayers); i++)
	{
		found = false;
		for (uint32_t ii = 0; ii < layerCount; ii++)
		{
			if (strcmp(validationLayers[i], layerProps[ii].layerName))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			asAlloc_LinearFree(layerProps);
			return false;
		}
	}
	asAlloc_LinearFree(layerProps);
	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vDebugCallback(
VkDebugReportFlagsEXT flags,
VkDebugReportObjectTypeEXT objType,
uint64_t srcObject,
size_t location,
int32_t msgCode,
const char* pLayerPrefix,
const char* pMsg,
void* pUserData) 
{
	asDebugLog("VK Validation Layer: [%s] Code %u : %s\n", pLayerPrefix, msgCode, pMsg);
	return VK_FALSE;
}
VkDebugReportCallbackEXT vDbgCallback;
#endif

vQueueFamilyIndices_t vFindQueueFamilyIndices(VkPhysicalDevice gpu)
{
	vQueueFamilyIndices_t result = (vQueueFamilyIndices_t) { UINT32_MAX, UINT32_MAX };
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, NULL);
	VkQueueFamilyProperties *queueFamilyProps = asAlloc_LinearMalloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount, 0);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilyProps);
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if (queueFamilyProps[i].queueCount > 0 && queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			result.graphicsIdx = i;

		VkBool32 present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, vMainScreen.surface, &present);
		if (queueFamilyProps[i].queueCount > 0 && present)
			result.presentIdx = i;

		if (vIsQueueFamilyComplete(result))
			break;
	}
	asAlloc_LinearFree(queueFamilyProps);
	return result;
}

int32_t vQueryAndRateSwapChainSupport(VkPhysicalDevice gpu, VkSurfaceKHR surf,
	VkSurfaceCapabilitiesKHR* pCaps, VkSurfaceFormatKHR *pSurfFormat, VkPresentModeKHR *pPresentMode)
{
	int32_t rating = 0;

	VkSurfaceCapabilitiesKHR surfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surf, &surfaceCaps);

	/*Formats*/
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surf, &formatCount, NULL);
	if (formatCount <= 0){/*No formats availible*/
		return -1;
	}
	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)asAlloc_LinearMalloc(sizeof(VkSurfaceFormatKHR)*formatCount, 0);
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surf, &formatCount, formats);

	/*Present Modes*/
	uint32_t modeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surf, &modeCount, NULL);
	if (modeCount <= 0) {/*No modes availible*/
		asAlloc_LinearFree(formats);
		return -1;
	}
	VkPresentModeKHR* modes = (VkPresentModeKHR*)asAlloc_LinearMalloc(sizeof(VkPresentModeKHR)*modeCount, 0);
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surf, &modeCount, modes);

	/*Pick best swapchain format*/
	VkSurfaceFormatKHR bestFormat = { VK_FORMAT_UNDEFINED, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) { /*No preferred format (choose what we want!)*/
		bestFormat = (VkSurfaceFormatKHR) { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		rating += 200;
	}
	else { /*We have to find a good format in a list of supported ones*/
		bestFormat = formats[0]; /*Fall back to first one*/
		for (uint32_t i = 0; i < formatCount; i++)
		{
			/*Our preferred format is avalible*/
			if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
				formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
				bestFormat = formats[i];
				break;
				rating += 100;
			}
		}		
	}

	/*Pick best mode*/
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < modeCount; i++)
	{
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			bestMode = modes[i];
			rating += 500;
			break;
		}
		else if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = modes[i];
			rating += 300;
			break;
		}
	}

	asAlloc_LinearFree(modes);
	asAlloc_LinearFree(formats);

	/*Additional benifits*/
	rating += surfaceCaps.maxImageExtent.width + surfaceCaps.maxImageExtent.height;

	/*Return best settings*/
	if (pCaps)
		*pCaps = surfaceCaps;
	if (pSurfFormat)
		*pSurfFormat = bestFormat;
	if (pPresentMode)
		*pPresentMode = bestMode;
	return rating;
}

bool vDeviceHasRequiredExtensions(VkPhysicalDevice gpu)
{
	uint32_t extCount;
	vkEnumerateDeviceExtensionProperties(gpu, NULL, &extCount, NULL);
	VkExtensionProperties* availible = (VkExtensionProperties*)asAlloc_LinearMalloc(sizeof(VkExtensionProperties)*extCount, 0);
	vkEnumerateDeviceExtensionProperties(gpu, NULL, &extCount, availible);

	bool foundAll = true;
	bool foundThis;
	for (uint32_t i = 0; i < ASARRAYSIZE(deviceReqExtensions); i++)
	{
		foundThis = false;
		for (uint32_t ii = 0; ii < extCount; ii++)
		{
			if (strcmp(deviceReqExtensions[i], availible[ii].extensionName) == 0)
			{
				foundThis = true;
				break;
			}
		}
		if(!foundThis)
			foundAll = false;
	}

	asAlloc_LinearFree(availible);
	return foundAll;
}

bool vDeviceHasRequiredFeatures(VkPhysicalDeviceFeatures *pFeatures)
{
	if (!pFeatures->imageCubeArray) /*Cubemap arrays must be supported*/
		return false;
	return true;
}

VkPhysicalDevice vPickBestDevice(VkPhysicalDevice* pGpus, uint32_t count)
{
	int64_t bestGPUIdx = -1;
	int64_t bestGPUScore = -1;
	int64_t currentGPUScore;
	for (uint32_t i = 0; i < count; i++)
	{
		currentGPUScore = 0;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(pGpus[i], &deviceProperties);
		vkGetPhysicalDeviceFeatures(pGpus[i], &deviceFeatures);

		/*Disqualifications*/
		if (!vDeviceHasRequiredFeatures(&deviceFeatures)) /*Device doesn't meet the minimum feature requirements*/
			continue;
		if (!vIsQueueFamilyComplete(vFindQueueFamilyIndices(pGpus[i]))) /*Queue families are incomplete*/
			continue;
		if (!vDeviceHasRequiredExtensions(pGpus[i])) /*Doesn't have the required extensions*/
			continue;
		int32_t vSwapchainScore = vQueryAndRateSwapChainSupport(pGpus[i], vMainScreen.surface, NULL, NULL, NULL);
		if (vSwapchainScore < 0) /*Swapchain is unusable*/
			continue;

		/*Benifits*/
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) /*Not an integrated GPU*/
			currentGPUScore += 10000;
		if (deviceFeatures.samplerAnisotropy)
			currentGPUScore += 100;
		vSwapchainScore += vSwapchainScore; /*How good is the swapchain support (Resolution, color, etc...)*/

		/*Set as GPU if its the best*/
		if (currentGPUScore > bestGPUScore)
		{
			bestGPUScore = currentGPUScore;
			bestGPUIdx = i;
		}

	}
	if (bestGPUIdx < 0)
		return VK_NULL_HANDLE;
	return pGpus[bestGPUIdx];
}

int32_t vFindMemoryType(const VkPhysicalDeviceMemoryProperties* pMemProps, uint32_t typeBitsReq, VkMemoryPropertyFlags requiredProps)
{
	for (uint32_t i = 0; i < pMemProps->memoryTypeCount; i++)
	{
		if ((typeBitsReq & (1 << i)) &&
			((pMemProps->memoryTypes[i].propertyFlags & requiredProps) == requiredProps))
		{
			return (int32_t)i;
		}
	}
	return -1;
}

int32_t asVkFindMemoryType(uint32_t typeBitsReq, VkMemoryPropertyFlags requiredProps)
{
	return vFindMemoryType(&asVkDeviceMemProps, typeBitsReq, requiredProps);
}

struct vMemoryAllocator_t
{
	uint32_t allocCount;
	VkDeviceSize *pTypeAllocationSizes;
};
void vMemoryAllocator_Init(struct vMemoryAllocator_t* pAllocator)
{
	pAllocator->allocCount = 0;
	pAllocator->pTypeAllocationSizes = (VkDeviceSize*)asMalloc(asVkDeviceMemProps.memoryTypeCount * sizeof(VkDeviceSize));
	memset(pAllocator->pTypeAllocationSizes, 0, asVkDeviceMemProps.memoryTypeCount * sizeof(VkDeviceSize));
}

void vMemoryAllocator_Shutdown(struct vMemoryAllocator_t* pAllocator)
{
	pAllocator->allocCount = 0;
	if(pAllocator->pTypeAllocationSizes)
		asFree(pAllocator->pTypeAllocationSizes);
}

struct vMemoryAllocator_t vMainAllocator;

void asVkAlloc(asVkAllocation_t *pMem, VkDeviceSize size, uint32_t type)
{
	vMainAllocator.allocCount++;
	vMainAllocator.pTypeAllocationSizes[type] += size;

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = type;
	if (vkAllocateMemory(asVkDevice, &allocInfo, AS_VK_MEMCB, &pMem->memHandle) != VK_SUCCESS)
		asFatalError("vkAllocateMemory() Failed to allocate memory");
	pMem->size = size;
	pMem->memType = type;
	pMem->offset = 0;
}
void asVkFree(asVkAllocation_t* pMem)
{
	vMainAllocator.allocCount--;
	vMainAllocator.pTypeAllocationSizes[pMem->memType] -= pMem->size;
	vkFreeMemory(asVkDevice, pMem->memHandle, AS_VK_MEMCB);
	pMem->memHandle = VK_NULL_HANDLE;
	pMem->size = 0;
	pMem->offset = 0;
}

/*Texture Stuff*/
/*Todo: Investigate how to better better apply DOD with this... (for now, it works)*/

struct vTexture_t
{
	asTextureType textureType;
	asGpuResourceUploadType cpuAccess;
	uint32_t activeImage; /*To support multiple in-flight frames*/
	asVkAllocation_t alloc[AS_VK_MAX_INFLIGHT];
	VkImage image[AS_VK_MAX_INFLIGHT];
	VkImageView view[AS_VK_MAX_INFLIGHT];
	VkSampler sampler;
};

void _invalidateTexture(struct vTexture_t* pTex)
{
	pTex->activeImage = 0;
	pTex->sampler = VK_NULL_HANDLE;
	for (int i = 0; i < AS_VK_MAX_INFLIGHT; i++)
	{
		pTex->alloc[i].memHandle = VK_NULL_HANDLE;
		pTex->image[i] = VK_NULL_HANDLE;
		pTex->view[i] = VK_NULL_HANDLE;
	}
}

void _destroyTexture(struct vTexture_t* pTex)
{
	for (int i = 0; i < AS_VK_MAX_INFLIGHT; i++)
	{
		if (pTex->alloc[i].memHandle != VK_NULL_HANDLE)
			asVkFree(&pTex->alloc[i]);
		if (pTex->image[i] != VK_NULL_HANDLE)
			vkDestroyImage(asVkDevice, pTex->image[i], AS_VK_MEMCB);
		if (pTex->view[i] != VK_NULL_HANDLE)
			vkDestroyImageView(asVkDevice, pTex->view[i], AS_VK_MEMCB);
	}
	if (pTex->sampler != VK_NULL_HANDLE)
		vkDestroySampler(asVkDevice, pTex->sampler, AS_VK_MEMCB);
	_invalidateTexture(pTex);
}

struct vTextureManager_t
{
	asHandleManager_t handleManager;
	struct vTexture_t* textures;
};
struct vTextureManager_t vMainTextureManager;

void vTextureManager_Init(struct vTextureManager_t* pMan)
{
	asHandleManagerCreate(&pMan->handleManager, AS_MAX_TEXTURES);
	pMan->textures = (struct vTexture_t*)asMalloc(sizeof(pMan->textures[0]) * AS_MAX_TEXTURES);
	for (int i = 0; i < AS_MAX_TEXTURES; i++)
		_invalidateTexture(&pMan->textures[i]);
}

void vTextureManager_Shutdown(struct vTextureManager_t* pMan)
{
	asHandleManagerDestroy(&pMan->handleManager);
	for (int i = 0; i < AS_MAX_TEXTURES; i++)
	{
		_destroyTexture(&pMan->textures[i]);
	}
	asFree(pMan->textures);
}

VkFormat vConvertToNativeFormat(asTextureFormat format)
{
	switch (format)
	{
	case AS_TEXTUREFORMAT_RGBA8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case AS_TEXTUREFORMAT_RGBA16_UNORM:
		return VK_FORMAT_R16G16B16A16_UNORM;
	case AS_TEXTUREFORMAT_RGBA16_SFLOAT:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case AS_TEXTUREFORMAT_R10G10B10A2_UNORM:
		return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case AS_TEXTUREFORMAT_R8_UNORM:
		return VK_FORMAT_R8_UNORM;
	case AS_TEXTUREFORMAT_R16_SFLOAT:
		return VK_FORMAT_R16_SFLOAT;
	case AS_TEXTUREFORMAT_R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case AS_TEXTUREFORMAT_R16G16_SFLOAT:
		return VK_FORMAT_R16G16_SFLOAT;
	case AS_TEXTUREFORMAT_BC1_UNORM_BLOCK:
		return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case AS_TEXTUREFORMAT_BC3_UNORM_BLOCK:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	case AS_TEXTUREFORMAT_BC5_UNORM_BLOCK:
		return VK_FORMAT_BC5_UNORM_BLOCK;
	case AS_TEXTUREFORMAT_BC6H_UFLOAT_BLOCK:
		return VK_FORMAT_BC6H_UFLOAT_BLOCK;
	case AS_TEXTUREFORMAT_BC7_UNORM_BLOCK:
		return VK_FORMAT_BC7_UNORM_BLOCK;
	case AS_TEXTUREFORMAT_DEPTH:
		return VK_FORMAT_D32_SFLOAT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

ASEXPORT uint32_t asGetDepthFormatSize(asTextureFormat preference)
{
	return 4; /*Most commonly supported depth format (D32, D24S8) size*/
}

VkImageType vConvertTextureTypeToImageType(asTextureType type)
{
	switch (type)
	{
	case AS_TEXTURETYPE_3D:
		return VK_IMAGE_TYPE_3D;
	default:
		return VK_IMAGE_TYPE_2D;
	}
}

VkImageViewType vConvertTextureTypeToViewType(asTextureType type)
{
	switch (type)
	{
	case AS_TEXTURETYPE_2D:
		return VK_IMAGE_VIEW_TYPE_2D;
	case AS_TEXTURETYPE_2DARRAY:
		return AS_TEXTURETYPE_2DARRAY;
	case AS_TEXTURETYPE_CUBE:
		return AS_TEXTURETYPE_CUBE;
	case AS_TEXTURETYPE_CUBEARRAY:
		return AS_TEXTURETYPE_CUBEARRAY;
	case AS_TEXTURETYPE_3D:
		return VK_IMAGE_VIEW_TYPE_3D;
	default:
		return 0;
	}
}

VkImageUsageFlags vDecodeTextureUsageFlags(uint32_t abstractFlags)
{
	VkImageUsageFlags initial = 0;
	if ((abstractFlags & AS_TEXTUREUSAGE_SAMPLED) == AS_TEXTUREUSAGE_SAMPLED)
		initial |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if ((abstractFlags & AS_TEXTUREUSAGE_RENDERTARGET) == AS_TEXTUREUSAGE_RENDERTARGET)
		initial |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if ((abstractFlags & AS_TEXTUREUSAGE_DEPTHBUFFER) == AS_TEXTUREUSAGE_DEPTHBUFFER)
		initial |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if ((abstractFlags & AS_TEXTUREUSAGE_TRANSFER_DST) == AS_TEXTUREUSAGE_TRANSFER_DST)
		initial |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if ((abstractFlags & AS_TEXTUREUSAGE_TRANSFER_SRC) == AS_TEXTUREUSAGE_TRANSFER_SRC)
		initial |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	return initial;
}

VkFilter vConvertTextureFilterToNativeFilter(asTextureFilterType type)
{
	switch (type)
	{
	case AS_TEXTUREFILTER_NEAREST:
		return VK_FILTER_NEAREST;
	default:
		return VK_FILTER_LINEAR;
	}
}

VkSamplerAddressMode vConvertTextureWrapToAddressMode(asTextureWrapType type)
{
	switch (type)
	{
	case AS_TEXTUREWRAP_CLAMP:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case AS_TEXTUREWRAP_MIRROR:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	default:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

ASEXPORT asTextureHandle_t asCreateTexture(asTextureDesc_t *pDesc)
{
	asTextureHandle_t hndl = (asTextureHandle_t) { 0 };
	vkDeviceWaitIdle(asVkDevice);
	hndl = asCreateHandle(&vMainTextureManager.handleManager);
	struct vTexture_t *pTex = &vMainTextureManager.textures[hndl._index];
	pTex->textureType = pDesc->type;
	pTex->cpuAccess = pDesc->cpuAccess;
	/*Image*/
	{
		VkImageCreateInfo createInfo = (VkImageCreateInfo) { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		createInfo.imageType = vConvertTextureTypeToImageType(pDesc->type);
		createInfo.format = vConvertToNativeFormat(pDesc->format);
		createInfo.extent.height = pDesc->height;
		createInfo.extent.width = pDesc->width;
		if (pDesc->type == AS_TEXTURETYPE_3D){
			createInfo.extent.depth = pDesc->depth;
			createInfo.arrayLayers = 1;
		}
		else{
			createInfo.extent.depth = 1;
			createInfo.arrayLayers = pDesc->depth;
		}
		createInfo.mipLevels = pDesc->mips;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		if (pDesc->cpuAccess == AS_GPURESOURCEACCESS_IMMUTABLE)
			createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		else
			createInfo.tiling = VK_IMAGE_TILING_LINEAR;
		createInfo.flags = 0;
		createInfo.usage = vDecodeTextureUsageFlags(pDesc->usageFlags);
		if ((pDesc->cpuAccess == AS_GPURESOURCEACCESS_IMMUTABLE) && pDesc->pInitialContentsBuffer)
			createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (pDesc->cpuAccess == AS_GPURESOURCEACCESS_IMMUTABLE){
			if (vkCreateImage(asVkDevice, &createInfo, AS_VK_MEMCB, &pTex->image[0]) != VK_SUCCESS)
				asFatalError("vkCreateImage() Failed to create an image");
			VkMemoryRequirements memReq;
			vkGetImageMemoryRequirements(asVkDevice, pTex->image[0], &memReq);
			asVkAlloc(&pTex->alloc[0], memReq.size, asVkFindMemoryType(memReq.memoryTypeBits,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
			vkBindImageMemory(asVkDevice, pTex->image[0], pTex->alloc[0].memHandle, pTex->alloc[0].offset);
		}
		else{
			for (int i = 0; i < AS_VK_MAX_INFLIGHT; i++){
				if (vkCreateImage(asVkDevice, &createInfo, AS_VK_MEMCB, &pTex->image[i]) != VK_SUCCESS)
					asFatalError("vkCreateImage() Failed to create an image");
				VkMemoryRequirements memReq;
				vkGetImageMemoryRequirements(asVkDevice, pTex->image[i], &memReq);
				asVkAlloc(&pTex->alloc[i], memReq.size, asVkFindMemoryType(memReq.memoryTypeBits,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
				vkBindImageMemory(asVkDevice, pTex->image[i], pTex->alloc[i].memHandle, pTex->alloc[i].offset);
			}
		}
	}
	/*View*/
	{
		const int count = pDesc->cpuAccess == AS_GPURESOURCEACCESS_IMMUTABLE ? 1 : AS_VK_MAX_INFLIGHT;
		for (int i = 0; i < count; i++){
			VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			createInfo.image = pTex->image[i];
			createInfo.viewType = vConvertTextureTypeToViewType(pDesc->type);
			createInfo.format = vConvertToNativeFormat(pDesc->format);
			createInfo.subresourceRange.aspectMask =
				pDesc->format == AS_TEXTUREFORMAT_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.layerCount = pDesc->type != AS_TEXTURETYPE_3D ? pDesc->depth : 1;
			createInfo.subresourceRange.levelCount = pDesc->mips;
			if (vkCreateImageView(asVkDevice, &createInfo, AS_VK_MEMCB, &pTex->view[i]) != VK_SUCCESS)
				asFatalError("vkCreateImageView() Failed to create an image view");
		}
	}
	/*Sampler*/
	{
		VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		createInfo.minLod = pDesc->minLod;
		createInfo.maxLod = pDesc->maxLod > pDesc->mips ? (float) pDesc->mips : pDesc->maxLod;
		createInfo.maxAnisotropy = (float)pDesc->maxAnisotropy;
		if(asVkDeviceFeatures.samplerAnisotropy)
			createInfo.anisotropyEnable = pDesc->maxAnisotropy > 1 ? VK_TRUE : VK_FALSE;
		createInfo.mipLodBias = 0.0f;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.minFilter = vConvertTextureFilterToNativeFilter(pDesc->minFilter);
		createInfo.magFilter = vConvertTextureFilterToNativeFilter(pDesc->magFilter);
		createInfo.addressModeU = vConvertTextureWrapToAddressMode(pDesc->uWrap);
		createInfo.addressModeV = vConvertTextureWrapToAddressMode(pDesc->vWrap);
		createInfo.addressModeW = vConvertTextureWrapToAddressMode(pDesc->wWrap);
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		if(vkCreateSampler(asVkDevice, &createInfo, AS_VK_MEMCB, &pTex->sampler) != VK_SUCCESS)
			asFatalError("vkCreateSampler() Failed to create a sampler");
	}
	/*Upload Data*/
	{
		if (pDesc->pInitialContentsBuffer && !((pDesc->usageFlags & AS_TEXTUREUSAGE_RENDERTARGET) == AS_TEXTUREUSAGE_RENDERTARGET))
		{
			if (pDesc->cpuAccess == AS_GPURESOURCEACCESS_IMMUTABLE) /*Requires Staging*/
			{
				/*Upload initial contents to GPU*/
				VkBuffer stagingBuffer;
				asVkAllocation_t stagingAlloc;
				{
					VkBufferCreateInfo bufferInfo = (VkBufferCreateInfo) { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
					bufferInfo.size = pDesc->initialContentsBufferSize;
					bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
					if (vkCreateBuffer(asVkDevice, &bufferInfo, AS_VK_MEMCB, &stagingBuffer) != VK_SUCCESS)
						asFatalError("vkCreateBuffer() Failed to create a staging buffer");
					VkMemoryRequirements memReq;
					vkGetBufferMemoryRequirements(asVkDevice, stagingBuffer, &memReq);
					asVkAlloc(&stagingAlloc, memReq.size, asVkFindMemoryType(memReq.memoryTypeBits,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
					vkBindBufferMemory(asVkDevice, stagingBuffer, stagingAlloc.memHandle, stagingAlloc.offset);
					void* pData;
					vkMapMemory(asVkDevice, stagingAlloc.memHandle, stagingAlloc.offset, pDesc->initialContentsBufferSize, 0, &pData);
					memcpy(pData, pDesc->pInitialContentsBuffer, pDesc->initialContentsBufferSize);
					vkUnmapMemory(asVkDevice, stagingAlloc.memHandle);
				}
				/*Copy staging contents into image*/
				{
					VkCommandBufferAllocateInfo cmdAlloc = (VkCommandBufferAllocateInfo) { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
					cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					cmdAlloc.commandPool = asVkMainCommandPool;
					cmdAlloc.commandBufferCount = 1;
					VkCommandBuffer tmpCmd;
					vkAllocateCommandBuffers(asVkDevice, &cmdAlloc, &tmpCmd);
					VkCommandBufferBeginInfo beginInfo = (VkCommandBufferBeginInfo){ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					vkBeginCommandBuffer(tmpCmd, &beginInfo);
					/*Transition image layout for copy*/
					VkImageMemoryBarrier toTransferDst = (VkImageMemoryBarrier){ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
					toTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					toTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					toTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					toTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					toTransferDst.image = pTex->image[0];
					toTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					toTransferDst.subresourceRange.baseMipLevel = 0;
					toTransferDst.subresourceRange.levelCount = pDesc->mips;
					toTransferDst.subresourceRange.baseArrayLayer = 0;
					toTransferDst.subresourceRange.layerCount = pDesc->type == AS_TEXTURETYPE_3D ? 1 : pDesc->depth;
					toTransferDst.srcAccessMask = 0;
					toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					vkCmdPipelineBarrier(tmpCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &toTransferDst);
					/*Copy each region*/
					for (uint32_t i = 0; i < pDesc->initialContentsRegionCount; i++)
					{
						VkBufferImageCopy cpy;
						cpy.bufferImageHeight = 0;
						cpy.bufferRowLength = 0;
						cpy.bufferOffset = pDesc->pInitialContentsRegions[i].bufferStart;
						cpy.imageExtent.width = pDesc->pInitialContentsRegions[i].extent[0];
						cpy.imageExtent.height = pDesc->pInitialContentsRegions[i].extent[1];
						cpy.imageExtent.depth = pDesc->pInitialContentsRegions[i].extent[2];
						cpy.imageOffset.x = pDesc->pInitialContentsRegions[i].offset[0];
						cpy.imageOffset.y = pDesc->pInitialContentsRegions[i].offset[1];
						cpy.imageOffset.z = pDesc->pInitialContentsRegions[i].offset[2];
						cpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; /*Uploading depth is unsupported*/
						cpy.imageSubresource.layerCount = pDesc->pInitialContentsRegions[i].layerCount;
						cpy.imageSubresource.baseArrayLayer = pDesc->pInitialContentsRegions[i].layer;
						cpy.imageSubresource.mipLevel = pDesc->pInitialContentsRegions[i].mipLevel;
						vkCmdCopyBufferToImage(tmpCmd, stagingBuffer, pTex->image[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);
					}
					/*Transition to shader input optimal layout*/
					VkImageMemoryBarrier toFinal = toTransferDst;
					toFinal.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					toFinal.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					toFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					toFinal.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(tmpCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &toFinal);
					/*Execute*/
					vkEndCommandBuffer(tmpCmd);
					VkSubmitInfo submitInfo = (VkSubmitInfo){ VK_STRUCTURE_TYPE_SUBMIT_INFO };
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &tmpCmd;
					vkQueueSubmit(asVkQueue_GFX, 1, &submitInfo, VK_NULL_HANDLE);
					vkQueueWaitIdle(asVkQueue_GFX);
					vkFreeCommandBuffers(asVkDevice, asVkMainCommandPool, 1, &tmpCmd);
				}
				/*Free staging data*/
				vkDestroyBuffer(asVkDevice, stagingBuffer, AS_VK_MEMCB);
				asVkFree(&stagingAlloc);
			}
			else 
			{
				/*Uploading currently unsupported for textures*/
			}
		}
	}
#if AS_VK_VALIDATION
	/*Debug Markers*/
	{
		/*
		if (pDesc->pDebugLabel)
		{
			VkDebugMarkerObjectNameInfoEXT imageInfo = { VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT };
			VkDebugMarkerObjectNameInfoEXT viewInfo = imageInfo;
			VkDebugMarkerObjectNameInfoEXT samplerInfo = viewInfo;
			samplerInfo.object = (uint64_t)pTex->sampler;
			samplerInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT;
			samplerInfo.pObjectName = pDesc->pDebugLabel;
			vkDebugMarkerSetObjectNameEXT(asVkDevice, &samplerInfo);
			const int count = pDesc->cpuAccess == AS_GPURESOURCEACCESS_IMMUTABLE ? 1 : AS_VK_MAX_INFLIGHT;
			for (int i = 0; i < count; i++) {
				imageInfo.object = (uint64_t)pTex->image[i];
				imageInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
				imageInfo.pObjectName = pDesc->pDebugLabel;
				vkDebugMarkerSetObjectNameEXT(asVkDevice, &imageInfo);
				viewInfo.object = (uint64_t)pTex->view[i];
				viewInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT;
				viewInfo.pObjectName = pDesc->pDebugLabel;
				vkDebugMarkerSetObjectNameEXT(asVkDevice, &viewInfo);
			}
		}
		*/
	}
#endif
	
	return hndl;
}

ASEXPORT void asReleaseTexture(asTextureHandle_t hndl)
{
	vkDeviceWaitIdle(asVkDevice);
	_destroyTexture(&vMainTextureManager.textures[hndl._index]);
	asDestroyHandle(&vMainTextureManager.handleManager, hndl);
}

/*Init and shutdown*/

void vScreenResourcesCreate(struct vScreenResources_t *pScreen, SDL_Window* pWindow)
{
	vkDeviceWaitIdle(asVkDevice);

	/*Set window*/
	pScreen->pWindow = pWindow;

	/*Recreate Surface if Necessary*/
	if(pScreen->surface != VK_NULL_HANDLE){
		if (!SDL_Vulkan_CreateSurface(pScreen->pWindow, asVkInstance, &pScreen->surface))
			asFatalError("SDL_Vulkan_CreateSurface() Failed to create surface");
	}
	/*Swapchain*/
	{
		vQueryAndRateSwapChainSupport(asVkPhysicalDevice, pScreen->surface,
			&pScreen->caps, &pScreen->surfFormat, &pScreen->presentMode);
		SDL_Vulkan_GetDrawableSize(pScreen->pWindow, (int*)&pScreen->extents.width, (int*)&pScreen->extents.height);

		uint32_t imageCount = pScreen->caps.minImageCount + 1;
		if (pScreen->caps.maxImageCount > 0 && imageCount > pScreen->caps.maxImageCount) {
			imageCount = pScreen->caps.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = (VkSwapchainCreateInfoKHR) { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		createInfo.surface = pScreen->surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = pScreen->surfFormat.format;
		createInfo.imageColorSpace = pScreen->surfFormat.colorSpace;
		createInfo.imageExtent = pScreen->extents;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.preTransform = pScreen->caps.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.clipped = VK_TRUE;
		createInfo.presentMode = pScreen->presentMode;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		vQueueFamilyIndices_t indices = vFindQueueFamilyIndices(asVkPhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsIdx, indices.presentIdx };
		if (queueFamilyIndices[0] != queueFamilyIndices[1]){
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		if (vkCreateSwapchainKHR(asVkDevice, &createInfo, AS_VK_MEMCB, &pScreen->swapchain) != VK_SUCCESS)
			asFatalError("vkCreateSwapchainKHR() Failed to create swapchain");
	}
	/*Get Swapchain Images*/
	{
		vkGetSwapchainImagesKHR(asVkDevice, pScreen->swapchain, &pScreen->imageCount, NULL);
		pScreen->pSwapImages = asMalloc(sizeof(VkImage) * pScreen->imageCount);
		vkGetSwapchainImagesKHR(asVkDevice, pScreen->swapchain, &pScreen->imageCount, pScreen->pSwapImages);
	}
}

void vScreenResourcesDestroy(struct vScreenResources_t *pScreen)
{
	/*Wait for device to come to a hault*/
	if(asVkDevice != VK_NULL_HANDLE)
		vkDeviceWaitIdle(asVkDevice);
	
	if (pScreen->pSwapImages)
		asFree(pScreen->pSwapImages);
	if (pScreen->swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(asVkDevice, pScreen->swapchain, AS_VK_MEMCB);
	if (pScreen->surface != VK_NULL_HANDLE)
	vkDestroySurfaceKHR(asVkInstance, pScreen->surface, NULL);
}

void asVkInit(asAppInfo_t *pAppInfo, asCfgFile_t* pConfig)
{
	asDebugLog("Starting Vulkan Backend...\n");
	/*Create Instance*/
	{
#if AS_VK_VALIDATION
		if (!vValidationLayersAvailible())
			asFatalError("Vulkan Validation layers requested but not avalible");
#endif
		unsigned int sdlExtCount;
		uint32_t extraExtCount = 1;
		if (!SDL_Vulkan_GetInstanceExtensions(asGetMainWindowPtr(), &sdlExtCount, NULL)) 
			asFatalError("SDL_Vulkan_GetInstanceExtensions() Failed to get instance extensions");
		const char** extensions;
		extensions = (const char**)asAlloc_LinearMalloc(sizeof(unsigned char*) * (sdlExtCount + extraExtCount), 0);
		extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
		SDL_Vulkan_GetInstanceExtensions(asGetMainWindowPtr(), &sdlExtCount, &extensions[extraExtCount]);
		for (uint32_t i = 0; i < sdlExtCount + extraExtCount; i++)
			asDebugLog("VK Extension: \"%s\" found\n", extensions[i]);

		VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.pEngineName = "astrengine";
		appInfo.engineVersion = VK_MAKE_VERSION(ASTRENGINE_VERSION_MAJOR, ASTRENGINE_VERSION_MINOR, ASTRENGINE_VERSION_PATCH);
		appInfo.pApplicationName = pAppInfo->pAppName;
		appInfo.applicationVersion = VK_MAKE_VERSION(pAppInfo->appVersion.major, pAppInfo->appVersion.minor, pAppInfo->appVersion.patch);
		appInfo.apiVersion = VK_VERSION_1_1;

		VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = (uint32_t)extraExtCount + sdlExtCount;
		createInfo.ppEnabledExtensionNames = extensions;
#if AS_VK_VALIDATION
		createInfo.ppEnabledLayerNames = validationLayers;
		createInfo.enabledLayerCount = ASARRAYSIZE(validationLayers);
#else 
		createInfo.ppEnabledLayerNames = NULL;
		createInfo.enabledLayerCount = 0;
#endif

		if(vkCreateInstance(&createInfo, AS_VK_MEMCB, &asVkInstance) != VK_SUCCESS)
			asFatalError("vkCreateInstance() Failed to create vulkan instance");
		asAlloc_LinearFree((void*)extensions);
	}
#if AS_VK_VALIDATION
	/*Setup Validation Debugging*/
	{
		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallback = 
			(PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(asVkInstance, "vkCreateDebugReportCallbackEXT");

		VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
		createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)vDebugCallback;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

		if (vkCreateDebugReportCallback)
			vkCreateDebugReportCallback(asVkInstance, &createInfo, AS_VK_MEMCB, &vDbgCallback);
		else
			asFatalError("Failed to find vkCreateDebugReportCallbackEXT()");
	}
#endif
	/*Create Starting Surface*/
	{
		if (!SDL_Vulkan_CreateSurface(asGetMainWindowPtr(), asVkInstance, &vMainScreen.surface))
			asFatalError("SDL_Vulkan_CreateSurface() Failed to create surface");
	}
	/*Pick Physical Device*/
	{
		uint32_t gpuCount;
		if(vkEnumeratePhysicalDevices(asVkInstance, &gpuCount, NULL) != VK_SUCCESS)
			asFatalError("Failed to find devices with vkEnumeratePhysicalDevices()");
		if(gpuCount == 0)
			asFatalError("No Supported Vulkan Compatible GPU found!");
		VkPhysicalDevice *gpus = asAlloc_LinearMalloc(gpuCount * sizeof(VkPhysicalDevice), 0);
		vkEnumeratePhysicalDevices(asVkInstance, &gpuCount, gpus);

		int preferredGPU = (int)asCfgGetNumber(pConfig, "GPUIndex", -1);
		if (preferredGPU > 0 && preferredGPU < (int)gpuCount) /*User selected GPU*/
		{
			asVkPhysicalDevice = gpus[preferredGPU];
		}
		else /*Try to find the best graphics card*/
		{
			asVkPhysicalDevice = vPickBestDevice(gpus, gpuCount);
			if(asVkPhysicalDevice == VK_NULL_HANDLE)
				asFatalError("Could not automatically find suitable graphics card");
		}
		vkGetPhysicalDeviceProperties(asVkPhysicalDevice, &asVkDeviceProperties);
		vkGetPhysicalDeviceFeatures(asVkPhysicalDevice, &asVkDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(asVkPhysicalDevice, &asVkDeviceMemProps);

		asAlloc_LinearFree(gpus);
	}
	/*Logical Device and Queues*/
	{
		asVkQueueFamilyIndices = vFindQueueFamilyIndices(asVkPhysicalDevice);
		if (!vIsQueueFamilyComplete(asVkQueueFamilyIndices))
			asFatalError("Device does not have the necessary queues for rendering");

		uint32_t uniqueIdxCount = 0;
		uint32_t uniqueIndices[2] = { UINT32_MAX, UINT32_MAX };
		/*Only create a list of unique indices for the queues*/
		{
			uint32_t nonUniqueIndices[2] = 
			{ asVkQueueFamilyIndices.graphicsIdx, 
			asVkQueueFamilyIndices.presentIdx };
			bool found;
			for (uint32_t i = 0; i < 2; i++) { /*Add items*/
				found = false;
				for (uint32_t ii = 0; ii < uniqueIdxCount + 1; ii++){ /*Search previous items*/
					if (uniqueIndices[ii] == nonUniqueIndices[i]){ /*Only add if*/
						found = true;
						break;
					}
				}
				if (!found){
					uniqueIndices[uniqueIdxCount] = nonUniqueIndices[i];
					uniqueIdxCount++;
				}
			}
		}
		VkDeviceQueueCreateInfo queueCreateInfos[2];
		float defaultPriority = 1.0f;
		for (uint32_t i = 0; i < uniqueIdxCount; i++)
		{
			queueCreateInfos[i] = (VkDeviceQueueCreateInfo) { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfos[i].queueCount = 1;
			queueCreateInfos[i].queueFamilyIndex = uniqueIndices[i];
			queueCreateInfos[i].pQueuePriorities = &defaultPriority;
		}

		/*Enabled features for the device*/
		VkPhysicalDeviceFeatures enabledFeatures = (VkPhysicalDeviceFeatures) { 0 };
		enabledFeatures.imageCubeArray = VK_TRUE;
		if(asVkDeviceFeatures.samplerAnisotropy)
			enabledFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.pQueueCreateInfos = queueCreateInfos;
		createInfo.queueCreateInfoCount = uniqueIdxCount;
		createInfo.pEnabledFeatures = &enabledFeatures;

		/*This is no-longer needed but is recomended for driver compatibility*/
#if AS_VK_VALIDATION
		createInfo.ppEnabledLayerNames = validationLayers;
		createInfo.enabledLayerCount = ASARRAYSIZE(validationLayers);
#else
		createInfo.ppEnabledLayerNames = NULL;
		createInfo.enabledLayerCount = 0;
#endif

		/*Extensions*/
		createInfo.enabledExtensionCount = ASARRAYSIZE(deviceReqExtensions);
		createInfo.ppEnabledExtensionNames = deviceReqExtensions;

		if(vkCreateDevice(asVkPhysicalDevice, &createInfo, AS_VK_MEMCB, &asVkDevice) != VK_SUCCESS)
			asFatalError("vkCreateDevice() failed to create the device");

		vkGetDeviceQueue(asVkDevice, asVkQueueFamilyIndices.graphicsIdx, 0, &asVkQueue_GFX);
		vkGetDeviceQueue(asVkDevice, asVkQueueFamilyIndices.presentIdx, 0, &asVkQueue_Present);
	}
	/*Memory Management*/
	{
		vMemoryAllocator_Init(&vMainAllocator);
	}
	/*Texture and Buffers*/
	{
		vTextureManager_Init(&vMainTextureManager);
	}
	/*Screen Resources*/
	{
		vScreenResourcesCreate(&vMainScreen, asGetMainWindowPtr());
	}
	/*Command Pools*/
	{
		VkCommandPoolCreateInfo createInfo = (VkCommandPoolCreateInfo){ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.queueFamilyIndex = asVkQueueFamilyIndices.graphicsIdx;
		createInfo.flags = 0;

		if (vkCreateCommandPool(asVkDevice, &createInfo, AS_VK_MEMCB, &asVkMainCommandPool) != VK_SUCCESS)
			asFatalError("vkCreateCommandPool() Failed to create a command pool");
	}
}

void asVkShutdown()
{
	if(asVkDevice != VK_NULL_HANDLE)
		vkDeviceWaitIdle(asVkDevice);

	if (asVkMainCommandPool != VK_NULL_HANDLE)
		vkDestroyCommandPool(asVkDevice, asVkMainCommandPool, AS_VK_MEMCB);
	vScreenResourcesDestroy(&vMainScreen);
	vTextureManager_Shutdown(&vMainTextureManager);
	vMemoryAllocator_Shutdown(&vMainAllocator);
	if(asVkDevice != VK_NULL_HANDLE)
		vkDestroyDevice(asVkDevice, AS_VK_MEMCB);
#if AS_VK_VALIDATION
	if (vDbgCallback != VK_NULL_HANDLE) {
		PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback =
			(PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(asVkInstance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallback(asVkInstance, vDbgCallback, AS_VK_MEMCB);
	}
#endif
	if (asVkInstance != VK_NULL_HANDLE)
		vkDestroyInstance(asVkInstance, AS_VK_MEMCB);
}
#endif