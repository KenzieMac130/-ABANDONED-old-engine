#include "asVulkanBackend.h"
#if ASTRENGINE_VK
#include <SDL_vulkan.h>
#include "../asRendererCore.h"

typedef struct{
	float customParams[AS_MAX_GLOBAL_CUSTOM_PARAMS][4];
	double timeElapsed;
	int debugMode;
} asUboGlobal;

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
	VkCommandBuffer *pPresentImageToScreenCmds;
	VkSemaphore swapImageAvailableSemaphores[AS_MAX_INFLIGHT];
	VkSemaphore blitFinishedSemaphores[AS_MAX_INFLIGHT];

	asTextureHandle_t compositeTexture;
	asTextureHandle_t depthTexture;
	VkFramebuffer simpleFrameBuffer;
	VkRenderPass simpleRenderPass;
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
VkQueue asVkQueue_Compute;
VkQueue asVkQueue_Transfer;

VkCommandPool asVkGeneralCommandPool;
uint32_t asVkCurrentFrame = 0;
VkFence asVkInFlightFences[AS_MAX_INFLIGHT];

typedef struct
{
	uint32_t graphicsIdx;
	uint32_t presentIdx;
	uint32_t computeIdx;
	uint32_t transferIdx;
} vQueueFamilyIndices_t;
vQueueFamilyIndices_t asVkQueueFamilyIndices;

bool vIsQueueFamilyComplete(vQueueFamilyIndices_t indices)
{
	return (indices.graphicsIdx != UINT32_MAX &&
		indices.transferIdx != UINT32_MAX &&
		indices.presentIdx != UINT32_MAX &&
		indices.computeIdx != UINT32_MAX);
}

const char* deviceReqExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#if AS_VK_VALIDATION
const char* validationLayers[] = {
	"VK_LAYER_LUNARG_standard_validation"
};
bool vMarkersExtensionFound;
PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName;

bool vValidationLayersAvailible()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	VkLayerProperties *layerProps = (VkLayerProperties*)asMalloc(sizeof(VkLayerProperties) * layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProps);
	for (uint32_t i = 0; i < layerCount; i++)
		asDebugLog("VK Layer: \"%s\" found", layerProps[i].layerName);
	bool found;
	for (uint32_t i = 0; i < ASARRAYLEN(validationLayers); i++)
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
			asFree(layerProps);
			return false;
		}
	}
	asFree(layerProps);
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
	asDebugWarning("VK Validation Layer: [%s] Code %u : %s", pLayerPrefix, msgCode, pMsg);
	return VK_FALSE;
}
VkDebugReportCallbackEXT vDbgCallback;
#endif

vQueueFamilyIndices_t vFindQueueFamilyIndices(VkPhysicalDevice gpu)
{
	vQueueFamilyIndices_t result = (vQueueFamilyIndices_t) { UINT32_MAX, UINT32_MAX, UINT32_MAX };
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, NULL);
	VkQueueFamilyProperties *queueFamilyProps = asMalloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilyProps);
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if (queueFamilyProps[i].queueCount > 0 && queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			result.graphicsIdx = i;
		if (queueFamilyProps[i].queueCount > 0 && queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			result.computeIdx = i;
		if (queueFamilyProps[i].queueCount > 0 && queueFamilyProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			result.transferIdx = i;

		VkBool32 present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, vMainScreen.surface, &present);
		if (queueFamilyProps[i].queueCount > 0 && present)
			result.presentIdx = i;

		if (vIsQueueFamilyComplete(result))
			break;
	}
	asFree(queueFamilyProps);
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
	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)asMalloc(sizeof(VkSurfaceFormatKHR)*formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surf, &formatCount, formats);

	/*Present Modes*/
	uint32_t modeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surf, &modeCount, NULL);
	if (modeCount <= 0) {/*No modes availible*/
		asFree(formats);
		return -1;
	}
	VkPresentModeKHR* modes = (VkPresentModeKHR*)asMalloc(sizeof(VkPresentModeKHR)*modeCount);
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

	asFree(modes);
	asFree(formats);

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
	VkExtensionProperties* availible = (VkExtensionProperties*)asMalloc(sizeof(VkExtensionProperties)*extCount);
	vkEnumerateDeviceExtensionProperties(gpu, NULL, &extCount, availible);

	bool foundAll = true;
	bool foundThis;
	for (uint32_t i = 0; i < ASARRAYLEN(deviceReqExtensions); i++)
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

	asFree(availible);
	return foundAll;
}

bool vDeviceHasRequiredFeatures(VkPhysicalDeviceFeatures *pFeatures)
{
	if (!pFeatures->imageCubeArray || /*Cubemap arrays must be supported*/
		!pFeatures->shaderFloat64) /*Float 64 for Shaders must be supported*/
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

void asVkMapMemory(asVkAllocation_t mem, VkDeviceSize offset, VkDeviceSize size, void** ppData)
{
	vkMapMemory(asVkDevice, mem.memHandle, mem.offset + offset, size, 0, ppData);
}

void asVkUnmapMemory(asVkAllocation_t mem)
{
	vkUnmapMemory(asVkDevice, mem.memHandle);
}

void asVkFlushMemory(asVkAllocation_t mem)
{
	VkMappedMemoryRange range = (VkMappedMemoryRange) { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
	range.memory = mem.memHandle;
	range.offset = mem.offset;
	range.size = mem.size;
	vkFlushMappedMemoryRanges(asVkDevice, 1, &range);
}

/*Texture Stuff*/

struct vTexture_t
{
	asTextureType textureType;
	asGpuResourceUploadType cpuAccess;
	asVkAllocation_t alloc;
	VkImage image;
	VkImageView view;
};

void _invalidateTexture(struct vTexture_t* pTex)
{
	pTex->alloc.memHandle = VK_NULL_HANDLE;
	pTex->image = VK_NULL_HANDLE;
	pTex->view = VK_NULL_HANDLE;
}

void _destroyTexture(struct vTexture_t* pTex)
{
	if (pTex->alloc.memHandle != VK_NULL_HANDLE)
		asVkFree(&pTex->alloc);
	if (pTex->image != VK_NULL_HANDLE)
		vkDestroyImage(asVkDevice, pTex->image, AS_VK_MEMCB);
	if (pTex->view != VK_NULL_HANDLE)
		vkDestroyImageView(asVkDevice, pTex->view, AS_VK_MEMCB);
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

VkFormat asVkConvertToNativePixelFormat(asColorFormat format)
{
	switch (format)
	{
	case AS_COLORFORMAT_DEPTH:
		return VK_FORMAT_D32_SFLOAT;
	case AS_COLORFORMAT_DEPTH_LP:
		return VK_FORMAT_D16_UNORM;
	case AS_COLORFORMAT_DEPTH_STENCIL:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	case AS_COLORFORMAT_RGBA8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case AS_COLORFORMAT_RGBA16_UNORM:
		return VK_FORMAT_R16G16B16A16_UNORM;
	case AS_COLORFORMAT_RGBA16_UINT:
		return VK_FORMAT_R16G16B16A16_UINT;
	case AS_COLORFORMAT_RGBA16_SFLOAT:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case AS_COLORFORMAT_RGBA32_SFLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case AS_COLORFORMAT_A2R10G10B10_UNORM:
		return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case AS_COLORFORMAT_A2R10G10B10_SNORM:
		return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
	case AS_COLORFORMAT_R8_UNORM:
		return VK_FORMAT_R8_UNORM;
	case AS_COLORFORMAT_R16_UNORM:
		return VK_FORMAT_R16_UNORM;
	case AS_COLORFORMAT_R16_SFLOAT:
		return VK_FORMAT_R16_SFLOAT;
	case AS_COLORFORMAT_R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case AS_COLORFORMAT_RG16_SFLOAT:
		return VK_FORMAT_R16G16_SFLOAT;
	case AS_COLORFORMAT_RG32_SFLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case AS_COLORFORMAT_RGB16_SFLOAT:
		return VK_FORMAT_R16G16B16_SFLOAT;
	case AS_COLORFORMAT_RGB32_SFLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case AS_COLORFORMAT_RGBA32_UINT:
		return VK_FORMAT_R32G32B32A32_UINT;
	case AS_COLORFORMAT_BC1_RGBA_UNORM_BLOCK:
		return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case AS_COLORFORMAT_BC3_UNORM_BLOCK:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	case AS_COLORFORMAT_BC5_UNORM_BLOCK:
		return VK_FORMAT_BC5_UNORM_BLOCK;
	case AS_COLORFORMAT_BC6H_UFLOAT_BLOCK:
		return VK_FORMAT_BC6H_UFLOAT_BLOCK;
	case AS_COLORFORMAT_BC7_UNORM_BLOCK:
		return VK_FORMAT_BC7_UNORM_BLOCK;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

VkShaderStageFlagBits asVkConvertToNativeStage(asShaderStage stage)
{
	switch (stage)
	{
	case AS_SHADERSTAGE_COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
	case AS_SHADERSTAGE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
	case AS_SHADERSTAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
	case AS_SHADERSTAGE_TESS_CONTROL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case AS_SHADERSTAGE_TESS_EVALUATION: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case AS_SHADERSTAGE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
	default: return VK_SHADER_STAGE_ALL;
	};
}

ASEXPORT uint32_t asGetDepthFormatSize(asColorFormat preference)
{
	if (preference == AS_COLORFORMAT_DEPTH_LP) { return 2; }
	else { return 4; } /*Most commonly supported depth format (D32, D24S8) size*/
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
		return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	case AS_TEXTURETYPE_CUBE:
		return VK_IMAGE_VIEW_TYPE_CUBE;
	case AS_TEXTURETYPE_CUBEARRAY:
		return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY ;
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
		createInfo.format = asVkConvertToNativePixelFormat(pDesc->format);
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
		if (pDesc->cpuAccess == AS_GPURESOURCEACCESS_DEVICE)
			createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		else
			createInfo.tiling = VK_IMAGE_TILING_LINEAR;
		createInfo.flags = 0;
		createInfo.usage = vDecodeTextureUsageFlags(pDesc->usageFlags);
		if ((pDesc->cpuAccess == AS_GPURESOURCEACCESS_DEVICE) && pDesc->pInitialContentsBuffer)
			createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (pDesc->cpuAccess != AS_GPURESOURCEACCESS_STREAM){
			if (vkCreateImage(asVkDevice, &createInfo, AS_VK_MEMCB, &pTex->image) != VK_SUCCESS)
				asFatalError("vkCreateImage() Failed to create an image");
			VkMemoryRequirements memReq;
			vkGetImageMemoryRequirements(asVkDevice, pTex->image, &memReq);
			asVkAlloc(&pTex->alloc, memReq.size, asVkFindMemoryType(memReq.memoryTypeBits,
				pDesc->cpuAccess == AS_GPURESOURCEACCESS_DEVICE ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
			vkBindImageMemory(asVkDevice, pTex->image, pTex->alloc.memHandle, pTex->alloc.offset);
		}
		else{
			if (vkCreateImage(asVkDevice, &createInfo, AS_VK_MEMCB, &pTex->image) != VK_SUCCESS)
				asFatalError("vkCreateImage() Failed to create an image");
			VkMemoryRequirements memReq;
			vkGetImageMemoryRequirements(asVkDevice, pTex->image, &memReq);
			asVkAlloc(&pTex->alloc, memReq.size, asVkFindMemoryType(memReq.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
			vkBindImageMemory(asVkDevice, pTex->image, pTex->alloc.memHandle, pTex->alloc.offset);
		}
	}
	/*View*/
	{
		VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		createInfo.image = pTex->image;
		createInfo.viewType = vConvertTextureTypeToViewType(pDesc->type);
		createInfo.format = asVkConvertToNativePixelFormat(pDesc->format);
		createInfo.subresourceRange.aspectMask =
			pDesc->format == AS_COLORFORMAT_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.layerCount = pDesc->type != AS_TEXTURETYPE_3D ? pDesc->depth : 1;
		createInfo.subresourceRange.levelCount = pDesc->mips;
		if (vkCreateImageView(asVkDevice, &createInfo, AS_VK_MEMCB, &pTex->view) != VK_SUCCESS)
			asFatalError("vkCreateImageView() Failed to create an image view");
	}
	/*Upload Data*/
	{
		if (pDesc->pInitialContentsBuffer && !((pDesc->usageFlags & AS_TEXTUREUSAGE_RENDERTARGET) == AS_TEXTUREUSAGE_RENDERTARGET))
		{
			if (pDesc->cpuAccess == AS_GPURESOURCEACCESS_DEVICE) /*Requires Staging*/
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
					cmdAlloc.commandPool = asVkGeneralCommandPool;
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
					toTransferDst.image = pTex->image;
					toTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					toTransferDst.subresourceRange.baseMipLevel = 0;
					toTransferDst.subresourceRange.levelCount = pDesc->mips;
					toTransferDst.subresourceRange.baseArrayLayer = 0;
					toTransferDst.subresourceRange.layerCount = pDesc->type == AS_TEXTURETYPE_3D ? 1 : pDesc->depth;
					toTransferDst.srcAccessMask = 0;
					toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					vkCmdPipelineBarrier(tmpCmd,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						0, 0, NULL, 0, NULL, 1, &toTransferDst);
					/*Copy each region*/
					const asTextureContentRegion_t* pRegions = pDesc->pInitialContentsRegions ?
						pDesc->pInitialContentsRegions : pDesc->arrInitialContentsRegions;
					for (uint32_t i = 0; i < pDesc->initialContentsRegionCount; i++)
					{
						VkBufferImageCopy cpy;
						cpy.bufferImageHeight = 0;
						cpy.bufferRowLength = 0;
						cpy.bufferOffset = pRegions[i].bufferStart;
						cpy.imageExtent.width = pRegions[i].extent[0];
						cpy.imageExtent.height = pRegions[i].extent[1];
						cpy.imageExtent.depth = pRegions[i].extent[2];
						cpy.imageOffset.x = pRegions[i].offset[0];
						cpy.imageOffset.y = pRegions[i].offset[1];
						cpy.imageOffset.z = pRegions[i].offset[2];
						cpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; /*Uploading depth is unsupported*/
						cpy.imageSubresource.layerCount = pRegions[i].layerCount;
						cpy.imageSubresource.baseArrayLayer = pRegions[i].layer;
						cpy.imageSubresource.mipLevel = pRegions[i].mipLevel;
						vkCmdCopyBufferToImage(tmpCmd, stagingBuffer, pTex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);
					}
					/*Transition to shader input optimal layout*/
					VkImageMemoryBarrier toFinal = toTransferDst;
					toFinal.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					toFinal.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					toFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					toFinal.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(tmpCmd,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
						0, 0, NULL, 0, NULL, 1, &toFinal);
					/*Execute*/
					vkEndCommandBuffer(tmpCmd);
					VkSubmitInfo submitInfo = (VkSubmitInfo){ VK_STRUCTURE_TYPE_SUBMIT_INFO };
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &tmpCmd;
					vkQueueSubmit(asVkQueue_Transfer, 1, &submitInfo, VK_NULL_HANDLE); 
					vkQueueWaitIdle(asVkQueue_Transfer); /*TODO: Resource Synchonization*/
					vkFreeCommandBuffers(asVkDevice, asVkGeneralCommandPool, 1, &tmpCmd);
				}
				/*Free staging data*/
				vkDestroyBuffer(asVkDevice, stagingBuffer, AS_VK_MEMCB);
				asVkFree(&stagingAlloc);
			}
			else 
			{
				/*Uploading is currently unsupported for dynamic textures... 
				you probably shouldn't be doing it this way either.
				upload to a staging buffer first then copy to a texture
				this should make it a lot faster to access texel*/
			}
		}
	}
#if AS_VK_VALIDATION
	/*Debug Markers*/
	{
		if (pDesc->pDebugLabel && vMarkersExtensionFound)
		{
			VkDebugMarkerObjectNameInfoEXT imageInfo = (VkDebugMarkerObjectNameInfoEXT){ VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT };
			VkDebugMarkerObjectNameInfoEXT viewInfo = imageInfo;
			imageInfo.object = (uint64_t)pTex->image;
			imageInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
			imageInfo.pObjectName = pDesc->pDebugLabel;
			vkDebugMarkerSetObjectName(asVkDevice, &imageInfo);
			viewInfo.object = (uint64_t)pTex->view;
			viewInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT;
			viewInfo.pObjectName = pDesc->pDebugLabel;
			vkDebugMarkerSetObjectName(asVkDevice, &viewInfo);
		}
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

VkImage asVkGetImageFromTexture(asTextureHandle_t hndl)
{
	return vMainTextureManager.textures[hndl._index].image;
}

VkImageView asVkGetViewFromTexture(asTextureHandle_t hndl)
{
	return vMainTextureManager.textures[hndl._index].view;
}

asVkAllocation_t asVkGetAllocFromTexture(asTextureHandle_t hndl)
{
	return vMainTextureManager.textures[hndl._index].alloc;
}

/*Buffer stuff*/

struct vBuffer_t
{
	asGpuResourceUploadType cpuAccess;
	asVkAllocation_t alloc;
	VkBuffer buffer;
};

void _invalidateBuffer(struct vBuffer_t* pBuf)
{
	pBuf->alloc.memHandle = VK_NULL_HANDLE;
	pBuf->buffer = VK_NULL_HANDLE;
}

void _destroyBuffer(struct vBuffer_t* pBuf)
{
	if (pBuf->alloc.memHandle != VK_NULL_HANDLE)
		asVkFree(&pBuf->alloc);
	if (pBuf->buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(asVkDevice, pBuf->buffer, AS_VK_MEMCB);
	_invalidateBuffer(pBuf);
}

struct vBufferManager_t
{
	asHandleManager_t handleManager;
	struct vBuffer_t* buffers;
};
struct vBufferManager_t vMainBufferManager;

void vBufferManager_Init(struct vBufferManager_t* pMan)
{
	asHandleManagerCreate(&pMan->handleManager, AS_MAX_TEXTURES);
	pMan->buffers = (struct vBuffer_t*)asMalloc(sizeof(pMan->buffers[0]) * AS_MAX_TEXTURES);
	for (int i = 0; i < AS_MAX_TEXTURES; i++)
		_invalidateBuffer(&pMan->buffers[i]);
}

void vBufferManager_Shutdown(struct vBufferManager_t* pMan)
{
	asHandleManagerDestroy(&pMan->handleManager);
	for (int i = 0; i < AS_MAX_TEXTURES; i++)
	{
		_destroyBuffer(&pMan->buffers[i]);
	}
	asFree(pMan->buffers);
}

VkBufferUsageFlags vDecodeBufferUsageFlags(uint32_t abstractFlags)
{
	VkImageUsageFlags initial = 0;
	if ((abstractFlags & AS_BUFFERUSAGE_TRANSFER_DST) == AS_BUFFERUSAGE_TRANSFER_DST)
		initial |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if ((abstractFlags & AS_BUFFERUSAGE_TRANSFER_SRC) == AS_BUFFERUSAGE_TRANSFER_SRC)
		initial |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if ((abstractFlags & AS_BUFFERUSAGE_INDEX) == AS_BUFFERUSAGE_INDEX)
		initial |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if ((abstractFlags & AS_BUFFERUSAGE_VERTEX) == AS_BUFFERUSAGE_VERTEX)
		initial |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if ((abstractFlags & AS_BUFFERUSAGE_INDIRECT) == AS_BUFFERUSAGE_INDIRECT)
		initial |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	if ((abstractFlags & AS_BUFFERUSAGE_UNIFORM) == AS_BUFFERUSAGE_UNIFORM)
		initial |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if ((abstractFlags & AS_BUFFERUSAGE_STORAGE) == AS_BUFFERUSAGE_STORAGE)
		initial |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	return initial;
}

ASEXPORT asBufferHandle_t asCreateBuffer(asBufferDesc_t *pDesc)
{
	asBufferHandle_t hndl = (asBufferHandle_t) { 0 };
	vkDeviceWaitIdle(asVkDevice);
	hndl = asCreateHandle(&vMainBufferManager.handleManager);
	struct vBuffer_t *pBuff = &vMainBufferManager.buffers[hndl._index];
	pBuff->cpuAccess = pDesc->cpuAccess;
	/*Buffer*/
	{
		VkBufferCreateInfo createInfo = (VkBufferCreateInfo) { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.size = pDesc->bufferSize;
		createInfo.usage = vDecodeBufferUsageFlags(pDesc->usageFlags);
		if ((pDesc->cpuAccess == AS_GPURESOURCEACCESS_DEVICE) && pDesc->pInitialContentsBuffer)
			createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (pDesc->cpuAccess != AS_GPURESOURCEACCESS_STREAM) {
			if (vkCreateBuffer(asVkDevice, &createInfo, AS_VK_MEMCB, &pBuff->buffer) != VK_SUCCESS)
				asFatalError("vkCreateBuffer() Failed to create a buffer");
			VkMemoryRequirements memReq;
			vkGetBufferMemoryRequirements(asVkDevice, pBuff->buffer, &memReq);
			asVkAlloc(&pBuff->alloc, memReq.size, asVkFindMemoryType(memReq.memoryTypeBits,
				pDesc->cpuAccess == AS_GPURESOURCEACCESS_DEVICE ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
			vkBindBufferMemory(asVkDevice, pBuff->buffer, pBuff->alloc.memHandle, pBuff->alloc.offset);
		}
		else {
			if (vkCreateBuffer(asVkDevice, &createInfo, AS_VK_MEMCB, &pBuff->buffer) != VK_SUCCESS)
				asFatalError("vkCreateBuffer() Failed to create a buffer");
			VkMemoryRequirements memReq;
			vkGetBufferMemoryRequirements(asVkDevice, pBuff->buffer, &memReq);
			asVkAlloc(&pBuff->alloc, memReq.size, asVkFindMemoryType(memReq.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
			vkBindBufferMemory(asVkDevice, pBuff->buffer, pBuff->alloc.memHandle, pBuff->alloc.offset);
		}
	}
	/*Upload Data*/
	{
		if (pDesc->pInitialContentsBuffer)
		{
			if (pDesc->cpuAccess == AS_GPURESOURCEACCESS_DEVICE) /*Requires Staging*/
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
				/*Copy staging contents into buffer*/
				{
					VkCommandBufferAllocateInfo cmdAlloc = (VkCommandBufferAllocateInfo) { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
					cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					cmdAlloc.commandPool = asVkGeneralCommandPool;
					cmdAlloc.commandBufferCount = 1;
					VkCommandBuffer tmpCmd;
					vkAllocateCommandBuffers(asVkDevice, &cmdAlloc, &tmpCmd);
					VkCommandBufferBeginInfo beginInfo = (VkCommandBufferBeginInfo) { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					vkBeginCommandBuffer(tmpCmd, &beginInfo);
					/*Transition image layout for copy*/
					VkBufferMemoryBarrier toTransferDst = (VkBufferMemoryBarrier) { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
					toTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					toTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					toTransferDst.buffer = pBuff->buffer;
					toTransferDst.offset = 0;
					toTransferDst.size = pDesc->bufferSize;
					toTransferDst.srcAccessMask = 0;
					toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					vkCmdPipelineBarrier(tmpCmd,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
						0, 0, NULL, 1, &toTransferDst, 0, NULL);
					/*Copy buffer region*/
					VkBufferCopy cpy;
					cpy.dstOffset = 0;
					cpy.srcOffset = 0;
					cpy.size = pDesc->bufferSize;
					vkCmdCopyBuffer(tmpCmd, stagingBuffer, pBuff->buffer, 1, &cpy);
					/*Transition to shader input optimal layout*/
					VkBufferMemoryBarrier toFinal = toTransferDst;
					toFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					toFinal.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(tmpCmd,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
						0, 0, NULL, 1, &toFinal, 0, NULL);
					/*Execute*/
					vkEndCommandBuffer(tmpCmd);
					VkSubmitInfo submitInfo = (VkSubmitInfo) { VK_STRUCTURE_TYPE_SUBMIT_INFO };
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &tmpCmd;
					vkQueueSubmit(asVkQueue_Transfer, 1, &submitInfo, VK_NULL_HANDLE);
					vkQueueWaitIdle(asVkQueue_Transfer); /*Todo: Proper Synchonization*/
					vkFreeCommandBuffers(asVkDevice, asVkGeneralCommandPool, 1, &tmpCmd);
				}
				/*Free staging data*/
				vkDestroyBuffer(asVkDevice, stagingBuffer, AS_VK_MEMCB);
				asVkFree(&stagingAlloc);
			}
			else /*No staging required*/
			{
				/*Simple map and memcpy*/
				void* pData;
				vkMapMemory(asVkDevice, pBuff->alloc.memHandle, pBuff->alloc.offset, pDesc->initialContentsBufferSize, 0, &pData);
				memcpy(pData, pDesc->pInitialContentsBuffer, pDesc->initialContentsBufferSize);
				vkUnmapMemory(asVkDevice, pBuff->alloc.memHandle);
			}
		}
	}
#if AS_VK_VALIDATION
	/*Debug Markers*/
	{
		if (pDesc->pDebugLabel && vMarkersExtensionFound)
		{
			VkDebugMarkerObjectNameInfoEXT bufferInfo = (VkDebugMarkerObjectNameInfoEXT) { VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT };
			bufferInfo.object = (uint64_t)pBuff->buffer;
			bufferInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT;
			bufferInfo.pObjectName = pDesc->pDebugLabel;
			vkDebugMarkerSetObjectName(asVkDevice, &bufferInfo);
		}
	}
#endif
	return hndl;
}

ASEXPORT void asReleaseBuffer(asBufferHandle_t hndl)
{
	vkDeviceWaitIdle(asVkDevice);
	_destroyBuffer(&vMainBufferManager.buffers[hndl._index]);
	asDestroyHandle(&vMainBufferManager.handleManager, hndl);
}

VkBuffer asVkGetBufferFromBuffer(asBufferHandle_t hndl)
{
	return vMainBufferManager.buffers[hndl._index].buffer;
}

asVkAllocation_t asVkGetAllocFromBuffer(asBufferHandle_t hndl)
{
	return vMainBufferManager.buffers[hndl._index].alloc;
}

VkRenderPass asVkGetSimpleDrawingRenderpass()
{
	return vMainScreen.simpleRenderPass;
}

VkFramebuffer asVkGetSimpleDrawingFramebuffer()
{
	return vMainScreen.simpleFrameBuffer;
}

VkSampler _vSamplerInterpolate;
VkSampler _vSamplerNoInterpolate;
VkSampler* asVkGetSimpleSamplerPtr(bool interpolate)
{
	return interpolate ? &_vSamplerInterpolate : &_vSamplerNoInterpolate;
}

/*Command Buffers*/
struct vPrimaryCommandBufferManager_t
{
	uint32_t count[AS_MAX_INFLIGHT];
	VkCommandBuffer *pBuffers[AS_MAX_INFLIGHT];
	VkCommandPool pools[AS_MAX_INFLIGHT];
	uint32_t maxBuffs;
};

void vPrimaryCommandBufferManager_Init(struct vPrimaryCommandBufferManager_t *pMan, uint32_t queueFamilyIndex, uint32_t count)
{
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		if (vkCreateCommandPool(asVkDevice, &poolInfo, AS_VK_MEMCB, &pMan->pools[i]) != VK_SUCCESS)
			asFatalError("vkCreateCommandPool() Failed to create primary command pool for manager");
		pMan->pBuffers[i] = asMalloc(sizeof(VkCommandBuffer) * count);
		pMan->maxBuffs = count;
		pMan->count[i] = 0;

		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = pMan->pools[i];
		allocInfo.commandBufferCount = count;
		if (vkAllocateCommandBuffers(asVkDevice, &allocInfo, pMan->pBuffers[i]) != VK_SUCCESS)
			asFatalError("vkAllocateCommandBuffers() Failed to allocate primary command buffers for manager");
	}
}

void vPrimaryCommandBufferManager_Shutdown(struct vPrimaryCommandBufferManager_t *pMan)
{
	for (int i = 0; i < AS_MAX_INFLIGHT; i++)
	{
		vkDestroyCommandPool(asVkDevice, pMan->pools[i], AS_VK_MEMCB);
		asFree(pMan->pBuffers[i]);
	}
}

void vPrimaryCommandBufferManager_SecureFrame(struct vPrimaryCommandBufferManager_t *pMan, uint32_t frame)
{
	vkResetCommandPool(asVkDevice, pMan->pools[frame], 0);
	pMan->count[frame] = 0;
}

VkCommandBuffer vPrimaryCommandBufferManager_GetNextCommand(struct vPrimaryCommandBufferManager_t *pMan)
{
	uint32_t slot;
	slot = pMan->count[asVkCurrentFrame];
/*NOT THREADSAFE TO RETRIEVE ON MULTIPLE THREADS!*/
	pMan->count[asVkCurrentFrame]+= 1;
	return pMan->pBuffers[asVkCurrentFrame][slot];
}

VkSubmitInfo vPrimaryCommandBufferManager_GenSubmitInfo(struct vPrimaryCommandBufferManager_t* pMan,
	uint32_t frame,
	VkPipelineStageFlags* pWaitStageMask,
	int32_t waitSemaphoreCount,
	VkSemaphore* pWaitSemaphores,
	int32_t signalSemaphoreCount,
	VkSemaphore* pSignalSemaphores)
{
	VkSubmitInfo submitInfo = (VkSubmitInfo){ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = pMan->count[frame];
	submitInfo.pCommandBuffers = pMan->pBuffers[frame];
	submitInfo.pWaitDstStageMask = pWaitStageMask;
	submitInfo.waitSemaphoreCount = waitSemaphoreCount;
	submitInfo.pWaitSemaphores = pWaitSemaphores;
	submitInfo.signalSemaphoreCount = signalSemaphoreCount;
	submitInfo.pSignalSemaphores = pSignalSemaphores;
	return submitInfo;
}

struct vPrimaryCommandBufferManager_t vMainGraphicsBufferManager;
struct vPrimaryCommandBufferManager_t vMainComputeBufferManager;

VkCommandBuffer asVkGetNextGraphicsCommandBuffer()
{
	return vPrimaryCommandBufferManager_GetNextCommand(&vMainGraphicsBufferManager);
}
VkCommandBuffer asVkGetNextComputeCommandBuffer()
{
	return vPrimaryCommandBufferManager_GetNextCommand(&vMainComputeBufferManager);
}


ASEXPORT asResults asGetRenderDimensions(int viewportId, bool dynamicRes, int32_t* pWidth, int32_t* pHeight)
{
	*pWidth = vMainScreen.extents.width;
	*pHeight = vMainScreen.extents.height;
	return AS_SUCCESS;
}

/*Global Shader UBO (Maybe move to render core)*/

asUboGlobal globalUboData;

ASEXPORT asResults asSetGlobalCustomShaderParam(int slot, float values[4])
{
	ASASSERT(slot < AS_MAX_GLOBAL_CUSTOM_PARAMS);
	memcpy(globalUboData.customParams[slot], values, sizeof(float) * 4);
	return AS_SUCCESS;
}

ASEXPORT asResults asSetGlobalShaderDebugMode(int mode)
{
	globalUboData.debugMode = mode;
	return AS_SUCCESS;
}

ASEXPORT asResults asSetGlobalShaderTime(double time)
{
	globalUboData.timeElapsed = time;
	return AS_SUCCESS;
}

/*Init and Shutdown*/

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
			createInfo.queueFamilyIndexCount = ASARRAYLEN(queueFamilyIndices);
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
	/*Synchronization*/
	{
		VkSemaphoreCreateInfo createInfo = (VkSemaphoreCreateInfo) { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		for (uint32_t i = 0; i < AS_MAX_INFLIGHT; i++)
		{
			if (vkCreateSemaphore(asVkDevice, &createInfo, AS_VK_MEMCB, &pScreen->swapImageAvailableSemaphores[i]) != VK_SUCCESS)
				asFatalError("vkCreateSemaphore() Failed to create semaphore");
			if (vkCreateSemaphore(asVkDevice, &createInfo, AS_VK_MEMCB, &pScreen->blitFinishedSemaphores[i]) != VK_SUCCESS)
				asFatalError("vkCreateSemaphore() Failed to create semaphore");
		}
	}
	/*Render Targets*/
	{
		/*Composite*/
		asTextureDesc_t compositeDesc = asTextureDesc_Init();
		compositeDesc.width = pScreen->extents.width;
		compositeDesc.height = pScreen->extents.height;
		compositeDesc.format = AS_COLORFORMAT_RGBA8_UNORM;
		compositeDesc.usageFlags |= AS_TEXTUREUSAGE_RENDERTARGET | AS_TEXTUREUSAGE_TRANSFER_SRC;
		compositeDesc.pDebugLabel = "Composite";
		pScreen->compositeTexture = asCreateTexture(&compositeDesc);
		/*Depth*/
		asTextureDesc_t depthDesc = asTextureDesc_Init();
		depthDesc.width = pScreen->extents.width;
		depthDesc.height = pScreen->extents.height;
		depthDesc.format = AS_COLORFORMAT_DEPTH;
		depthDesc.usageFlags |= AS_TEXTUREUSAGE_DEPTHBUFFER;
		depthDesc.pDebugLabel = "Depth";
		pScreen->depthTexture = asCreateTexture(&depthDesc);
	}
	/*Simple Drawing Renderpass*/
	{
		VkAttachmentDescription attachments[] = {
			(VkAttachmentDescription){ /*Color Buffer*/
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
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

		VkRenderPassCreateInfo createInfo = (VkRenderPassCreateInfo){VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		createInfo.attachmentCount = ASARRAYLEN(attachments);
		createInfo.pAttachments = attachments;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(asVkDevice, &createInfo, AS_VK_MEMCB, &pScreen->simpleRenderPass) != VK_SUCCESS)
			asFatalError("vkCreateRenderPass() Failed to create pScreen->simpleRenderPass");
	}
	/*Simple Drawing Framebuffer*/
	{
		VkImageView attachments[] = {
			asVkGetViewFromTexture(pScreen->compositeTexture),
			asVkGetViewFromTexture(pScreen->depthTexture)
		};
		VkFramebufferCreateInfo createInfo = (VkFramebufferCreateInfo){VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		createInfo.renderPass = pScreen->simpleRenderPass;
		createInfo.attachmentCount = ASARRAYLEN(attachments);
		createInfo.pAttachments = attachments;
		createInfo.width = pScreen->extents.width;
		createInfo.height = pScreen->extents.height;
		createInfo.layers = 1;

		if(vkCreateFramebuffer(asVkDevice, &createInfo, AS_VK_MEMCB, &pScreen->simpleFrameBuffer) != VK_SUCCESS)
			asFatalError("vkCreateFramebuffer() Failed to create pScreen->simpleFrameBuffer");
	}
	/*Screen Presentation Commands*/
	{
		pScreen->pPresentImageToScreenCmds = asMalloc(sizeof(VkCommandBuffer) * pScreen->imageCount);

		VkCommandBufferAllocateInfo allocInfo = (VkCommandBufferAllocateInfo){VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.commandPool = asVkGeneralCommandPool;
		allocInfo.commandBufferCount = pScreen->imageCount;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		if(vkAllocateCommandBuffers(asVkDevice, &allocInfo, pScreen->pPresentImageToScreenCmds) != VK_SUCCESS)
			asFatalError("vkAllocateCommandBuffers() Failed to allocate presentation commands");

		for (uint32_t i = 0; i < pScreen->imageCount; i++)
		{
			VkCommandBufferBeginInfo beginInfo = (VkCommandBufferBeginInfo) { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			vkBeginCommandBuffer(pScreen->pPresentImageToScreenCmds[i], &beginInfo);
			/*Transfer swap image and composite for blit*/
			VkImageMemoryBarrier toTransferSrc = (VkImageMemoryBarrier){ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			toTransferSrc.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			toTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			toTransferSrc.srcQueueFamilyIndex = asVkQueueFamilyIndices.graphicsIdx;
			toTransferSrc.dstQueueFamilyIndex = asVkQueueFamilyIndices.graphicsIdx;
			toTransferSrc.image = asVkGetImageFromTexture(pScreen->compositeTexture);
			toTransferSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toTransferSrc.subresourceRange.baseMipLevel = 0;
			toTransferSrc.subresourceRange.levelCount = 1;
			toTransferSrc.subresourceRange.baseArrayLayer = 0;
			toTransferSrc.subresourceRange.layerCount = 1;
			toTransferSrc.srcAccessMask = 0;
			toTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			VkImageMemoryBarrier toTransferDst = (VkImageMemoryBarrier) { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			toTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			toTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			toTransferDst.srcQueueFamilyIndex = asVkQueueFamilyIndices.presentIdx;
			toTransferDst.dstQueueFamilyIndex = asVkQueueFamilyIndices.graphicsIdx;
			toTransferDst.image = pScreen->pSwapImages[i];
			toTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toTransferDst.subresourceRange.baseMipLevel = 0;
			toTransferDst.subresourceRange.levelCount = 1;
			toTransferDst.subresourceRange.baseArrayLayer = 0;
			toTransferDst.subresourceRange.layerCount = 1;
			toTransferDst.srcAccessMask = 0;
			toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			VkImageMemoryBarrier barriers[2] = { toTransferSrc, toTransferDst };
			vkCmdPipelineBarrier(pScreen->pPresentImageToScreenCmds[i],
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 2, barriers);

			/*Blit composite onto swapchain*/
			VkImageBlit region = (VkImageBlit) { 0 };
			region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.dstSubresource.layerCount = 1;
			region.srcSubresource = region.dstSubresource;
			region.dstOffsets[1].x = pScreen->extents.width;
			region.dstOffsets[1].y = pScreen->extents.height;
			region.dstOffsets[1].z = 1;
			region.srcOffsets[1] = region.dstOffsets[1];
			vkCmdBlitImage(pScreen->pPresentImageToScreenCmds[i],
				asVkGetImageFromTexture(pScreen->compositeTexture), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				pScreen->pSwapImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &region, VK_FILTER_LINEAR);

			/*Transfer swap image for present*/
			VkImageMemoryBarrier toPresent = (VkImageMemoryBarrier) { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			toPresent.srcQueueFamilyIndex = asVkQueueFamilyIndices.graphicsIdx;
			toPresent.dstQueueFamilyIndex = asVkQueueFamilyIndices.presentIdx;
			toPresent.image = pScreen->pSwapImages[i];
			toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toPresent.subresourceRange.baseMipLevel = 0;
			toPresent.subresourceRange.levelCount = 1;
			toPresent.subresourceRange.baseArrayLayer = 0;
			toPresent.subresourceRange.layerCount = 1;
			toPresent.srcAccessMask = 0;
			toPresent.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(pScreen->pPresentImageToScreenCmds[i],
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &toPresent);
		
			vkEndCommandBuffer(pScreen->pPresentImageToScreenCmds[i]);
		}
	}
}

void vScreenResourcesDestroy(struct vScreenResources_t *pScreen)
{
	/*Wait for device to come to a hault*/
	if(asVkDevice != VK_NULL_HANDLE)
		vkDeviceWaitIdle(asVkDevice);

	vkFreeCommandBuffers(asVkDevice, asVkGeneralCommandPool, pScreen->imageCount, pScreen->pPresentImageToScreenCmds);
	asFree(pScreen->pPresentImageToScreenCmds);

	vkDestroyFramebuffer(asVkDevice, pScreen->simpleFrameBuffer, AS_VK_MEMCB);
	vkDestroyRenderPass(asVkDevice, pScreen->simpleRenderPass, AS_VK_MEMCB);

	for (uint32_t i = 0; i < AS_MAX_INFLIGHT; i++){
		if (pScreen->swapImageAvailableSemaphores[i] == VK_NULL_HANDLE)
			break;
		vkDestroySemaphore(asVkDevice, pScreen->swapImageAvailableSemaphores[i], AS_VK_MEMCB);
		vkDestroySemaphore(asVkDevice, pScreen->blitFinishedSemaphores[i], AS_VK_MEMCB);
	}

	asReleaseTexture(pScreen->compositeTexture);
	asReleaseTexture(pScreen->depthTexture);
	
	if (pScreen->pSwapImages)
		asFree(pScreen->pSwapImages);
	if (pScreen->swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(asVkDevice, pScreen->swapchain, AS_VK_MEMCB);
	if (pScreen->surface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(asVkInstance, pScreen->surface, AS_VK_MEMCB);
}

void asVkInit(asAppInfo_t *pAppInfo, int32_t deviceIndex)
{
	asDebugLog("Starting Vulkan Backend...");
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
		extensions = (const char**)asMalloc(sizeof(unsigned char*) * (sdlExtCount + extraExtCount));
		extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
		SDL_Vulkan_GetInstanceExtensions(asGetMainWindowPtr(), &sdlExtCount, &extensions[extraExtCount]);
		for (uint32_t i = 0; i < sdlExtCount + extraExtCount; i++)
			asDebugLog("VK Extension: \"%s\" found", extensions[i]);

		VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.pEngineName = "astrengine";
		appInfo.engineVersion = VK_MAKE_VERSION(ASTRENGINE_VERSION_MAJOR, ASTRENGINE_VERSION_MINOR, ASTRENGINE_VERSION_PATCH);
		appInfo.pApplicationName = pAppInfo->pAppName;
		appInfo.applicationVersion = VK_MAKE_VERSION(pAppInfo->appVersion.major, pAppInfo->appVersion.minor, pAppInfo->appVersion.patch);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = (uint32_t)extraExtCount + sdlExtCount;
		createInfo.ppEnabledExtensionNames = extensions;
#if AS_VK_VALIDATION
		createInfo.ppEnabledLayerNames = validationLayers;
		createInfo.enabledLayerCount = ASARRAYLEN(validationLayers);
#else 
		createInfo.ppEnabledLayerNames = NULL;
		createInfo.enabledLayerCount = 0;
#endif

		if(vkCreateInstance(&createInfo, AS_VK_MEMCB, &asVkInstance) != VK_SUCCESS)
			asFatalError("vkCreateInstance() Failed to create vulkan instance");
		asFree((void*)extensions);
	}
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
		if(!gpuCount)
			asFatalError("No Supported Vulkan Compatible GPU found!");
		VkPhysicalDevice *gpus = asMalloc(gpuCount * sizeof(VkPhysicalDevice));
		vkEnumeratePhysicalDevices(asVkInstance, &gpuCount, gpus);

		int preferredGPU = deviceIndex;
		if (preferredGPU >= 0 && preferredGPU < (int)gpuCount) /*User selected GPU*/
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

		asFree(gpus);
	}
	/*Find Extensions*/
	{
		uint32_t extCount;
		vkEnumerateDeviceExtensionProperties(asVkPhysicalDevice, NULL, &extCount, NULL);
		VkExtensionProperties* availible = (VkExtensionProperties*)asMalloc(sizeof(VkExtensionProperties)*extCount);
		vkEnumerateDeviceExtensionProperties(asVkPhysicalDevice, NULL, &extCount, availible);
		for (uint32_t i = 0; i < ASARRAYLEN(deviceReqExtensions); i++)
		{
			for (uint32_t ii = 0; ii < extCount; ii++)
			{
#if AS_VK_VALIDATION
				/*Debug Markers Extension*/
				if (strcmp(availible[ii].extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
				{
					vMarkersExtensionFound = true;
				}
#endif
			}
		}
		asFree(availible);
	}
#if AS_VK_VALIDATION
	/*Setup Validation Debug Callback*/
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

		/*Object Debug Labels*/
		if (vMarkersExtensionFound)
		{
			vkDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetInstanceProcAddr(asVkInstance, "vkDebugMarkerSetObjectNameEXT");
		}
		else
		{
			vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
		}
	}
#endif
	/*Logical Device and Queues (Must be updated when you add more queues)*/
	{
		asVkQueueFamilyIndices = vFindQueueFamilyIndices(asVkPhysicalDevice);
		if (!vIsQueueFamilyComplete(asVkQueueFamilyIndices))
			asFatalError("Device does not have the necessary queues for rendering");

		uint32_t uniqueIdxCount = 0;
		uint32_t uniqueIndices[3] = { UINT32_MAX, UINT32_MAX, UINT32_MAX };
		/*ONLY create a list of unique indices*/
		{
			uint32_t nonUniqueIndices[3] = 
			{ asVkQueueFamilyIndices.graphicsIdx, 
			asVkQueueFamilyIndices.presentIdx,
			asVkQueueFamilyIndices.computeIdx };
			bool found;
			for (uint32_t i = 0; i < ASARRAYLEN(nonUniqueIndices); i++) { /*Add items*/
				found = false;
				for (uint32_t ii = 0; ii < uniqueIdxCount + 1; ii++){ /*Search previous items*/
					if (uniqueIndices[ii] == nonUniqueIndices[i]){ /*Only add if unique*/
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
		VkDeviceQueueCreateInfo queueCreateInfos[3];
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
		enabledFeatures.shaderFloat64 = VK_TRUE;
		if(asVkDeviceFeatures.samplerAnisotropy)
			enabledFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.pQueueCreateInfos = queueCreateInfos;
		createInfo.queueCreateInfoCount = uniqueIdxCount;
		createInfo.pEnabledFeatures = &enabledFeatures;

		/*This is no-longer needed but is recomended for driver compatibility*/
#if AS_VK_VALIDATION
		createInfo.ppEnabledLayerNames = validationLayers;
		createInfo.enabledLayerCount = ASARRAYLEN(validationLayers);
#else
		createInfo.ppEnabledLayerNames = NULL;
		createInfo.enabledLayerCount = 0;
#endif

		/*Extensions*/
		createInfo.enabledExtensionCount = ASARRAYLEN(deviceReqExtensions);
		createInfo.ppEnabledExtensionNames = deviceReqExtensions;

		if(vkCreateDevice(asVkPhysicalDevice, &createInfo, AS_VK_MEMCB, &asVkDevice) != VK_SUCCESS)
			asFatalError("vkCreateDevice() failed to create the device");

		vkGetDeviceQueue(asVkDevice, asVkQueueFamilyIndices.graphicsIdx, 0, &asVkQueue_GFX);
		vkGetDeviceQueue(asVkDevice, asVkQueueFamilyIndices.presentIdx, 0, &asVkQueue_Present);
		vkGetDeviceQueue(asVkDevice, asVkQueueFamilyIndices.computeIdx, 0, &asVkQueue_Compute);
		vkGetDeviceQueue(asVkDevice, asVkQueueFamilyIndices.transferIdx, 0, &asVkQueue_Transfer);
	}
	/*Render Loop Synchronization*/
	{
		VkFenceCreateInfo fenceInfo = (VkFenceCreateInfo){ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (uint32_t i = 0; i < AS_MAX_INFLIGHT; i++)
		{
			if(vkCreateFence(asVkDevice, &fenceInfo, AS_VK_MEMCB, &asVkInFlightFences[i]) != VK_SUCCESS)
				asFatalError("vkCreateFence() Failed to create inflight fence");
		}
	}
	/*Command Pools*/
	{
		VkCommandPoolCreateInfo createInfo = (VkCommandPoolCreateInfo) { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.queueFamilyIndex = asVkQueueFamilyIndices.graphicsIdx;
		createInfo.flags = 0;

		if (vkCreateCommandPool(asVkDevice, &createInfo, AS_VK_MEMCB, &asVkGeneralCommandPool) != VK_SUCCESS)
			asFatalError("vkCreateCommandPool() Failed to create general command pool");

		vPrimaryCommandBufferManager_Init(&vMainGraphicsBufferManager, asVkQueueFamilyIndices.graphicsIdx, 64);
		vPrimaryCommandBufferManager_Init(&vMainComputeBufferManager, asVkQueueFamilyIndices.computeIdx, 32);
	}
	/*Memory Management*/
	{
		vMemoryAllocator_Init(&vMainAllocator);
	}
	/*Texture, Buffers and Shaders*/
	{
		vTextureManager_Init(&vMainTextureManager);
		vBufferManager_Init(&vMainBufferManager);
	}
	/*Simple Texture Samplers*/
	{
		VkSamplerCreateInfo createInfo = (VkSamplerCreateInfo){ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.maxAnisotropy = 16.0f;
		createInfo.compareEnable = VK_FALSE;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.maxLod = 1.0f;
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.0f;
		createInfo.mipLodBias = 0.0f;
		vkCreateSampler(asVkDevice, &createInfo, AS_VK_MEMCB, &_vSamplerInterpolate);
		createInfo.magFilter = VK_FILTER_NEAREST;
		vkCreateSampler(asVkDevice, &createInfo, AS_VK_MEMCB, &_vSamplerNoInterpolate);
	}
	/*Screen Resources*/
	{
		vScreenResourcesCreate(&vMainScreen, asGetMainWindowPtr());
	}
}

void asVkWindowResize()
{
	vScreenResourcesDestroy(&vMainScreen);
	vScreenResourcesCreate(&vMainScreen, asGetMainWindowPtr());
}

void vPresentFrame(struct vScreenResources_t *pScreen)
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(asVkDevice, pScreen->swapchain, UINT64_MAX,
		pScreen->swapImageAvailableSemaphores[asVkCurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		asVkWindowResize();
		return;
	}

	/*Submit Screen*/
	VkSubmitInfo submitInfo = (VkSubmitInfo) { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &pScreen->swapImageAvailableSemaphores[asVkCurrentFrame];
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &pScreen->pPresentImageToScreenCmds[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &pScreen->blitFinishedSemaphores[asVkCurrentFrame];
	vkResetFences(asVkDevice, 1, &asVkInFlightFences[asVkCurrentFrame]);
	if (vkQueueSubmit(asVkQueue_GFX, 1, &submitInfo, asVkInFlightFences[asVkCurrentFrame]) != VK_SUCCESS)
		asFatalError("vkQueueSubmit() Failed to blit to swapchain");
	VkPresentInfoKHR presentInfo = (VkPresentInfoKHR){ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &pScreen->blitFinishedSemaphores[asVkCurrentFrame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &pScreen->swapchain;
	presentInfo.pImageIndices = &imageIndex;
	result = vkQueuePresentKHR(asVkQueue_Present, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		asVkWindowResize();
	}
}

void asVkInitFrame()
{
	/*Wait if Frame is in-flight*/
	vkWaitForFences(asVkDevice, 1, &asVkInFlightFences[asVkCurrentFrame], VK_TRUE, UINT64_MAX);

	/*Secure Frame Resources (reset if used)*/
	vPrimaryCommandBufferManager_SecureFrame(&vMainGraphicsBufferManager, asVkCurrentFrame);
	vPrimaryCommandBufferManager_SecureFrame(&vMainComputeBufferManager, asVkCurrentFrame);
}

void asVkDrawFrame()
{
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo gfxSubmitInfo = vPrimaryCommandBufferManager_GenSubmitInfo(&vMainGraphicsBufferManager, asVkCurrentFrame, waitStages, 0, NULL, 0, NULL);
	if (vkQueueSubmit(asVkQueue_GFX, 1, &gfxSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		asFatalError("vkQueueSubmit() Failed to submit graphics commands");

	vPresentFrame(&vMainScreen);

	/*Next Frame*/
	asVkCurrentFrame = (asVkCurrentFrame + 1) % AS_MAX_INFLIGHT;
}

void asVkInitShutdown()
{
	if (asVkDevice != VK_NULL_HANDLE)
		vkDeviceWaitIdle(asVkDevice);

	vPrimaryCommandBufferManager_Shutdown(&vMainGraphicsBufferManager);
	vPrimaryCommandBufferManager_Shutdown(&vMainComputeBufferManager);
}

void asVkFinalShutdown()
{
	vScreenResourcesDestroy(&vMainScreen);
	vkDestroySampler(asVkDevice, _vSamplerInterpolate, AS_VK_MEMCB);
	vkDestroySampler(asVkDevice, _vSamplerNoInterpolate, AS_VK_MEMCB);
	vBufferManager_Shutdown(&vMainBufferManager);
	vTextureManager_Shutdown(&vMainTextureManager);
	vMemoryAllocator_Shutdown(&vMainAllocator);

	if (asVkGeneralCommandPool != VK_NULL_HANDLE)
		vkDestroyCommandPool(asVkDevice, asVkGeneralCommandPool, AS_VK_MEMCB);

	for (uint32_t i = 0; i < AS_MAX_INFLIGHT; i++){
		if (asVkInFlightFences[i] != VK_NULL_HANDLE)
			vkDestroyFence(asVkDevice, asVkInFlightFences[i], AS_VK_MEMCB);
	}

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