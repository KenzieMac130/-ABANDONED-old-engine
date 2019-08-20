#pragma once

#include "../asCommon.h"
#include "../asRendererCore.h"
#if ASTRENGINE_VK
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif 
/**
* @file
* @brief Low level vulkan functionality
* @warning Everything in this file the user should avoid touching (in fact it won't be exported)
*/

#ifdef NDEBUG 
#define AS_VK_VALIDATION false
#else
#define AS_VK_VALIDATION true
#endif

#define AS_VK_MEMCB NULL

/**
* @brief current inflight frame
* @warning DO NOT WRITE!
*/
extern uint32_t asVkCurrentFrame;

/**
* @brief the global vulkan instance
*/
extern VkInstance asVkInstance;
/**
* @brief the global selected physical device
*/
extern VkPhysicalDevice asVkPhysicalDevice;
/**
* @brief properties of the GPU 
*/
extern VkPhysicalDeviceProperties asVkDeviceProperties;
/**
* @brief features of the GPU
*/
extern VkPhysicalDeviceFeatures asVkDeviceFeatures;
/**
* @brief memory properties of the GPU
*/
extern VkPhysicalDeviceMemoryProperties asVkDeviceMemProps;
/**
* @brief the global selected physical device
*/
extern VkDevice asVkDevice;

/**
* @brief startup vulkan backend
* creates an instance, device, memory allocator, etc...
*/
void asVkInit(asLinearMemoryAllocator_t* pLinearAllocator, asAppInfo_t *pAppInfo, asCfgFile_t* pConfig);
/**
* @brief shutdown vulkan backend
* destroys every vulkan resource for a clean shutdown
*/
void asVkShutdown();

/**
* @brief draw a single frame
*/
void asVkDrawFrame();

/**
* @brief resize a window
*/
void asVkWindowResize(asLinearMemoryAllocator_t* pLinearAllocator);

/**
* @brief a memory allocation inside the vulkan backend
* is handled through the custom vulkan allocator
*/
typedef struct {
	VkDeviceMemory memHandle;
	uint32_t memType;
	VkDeviceSize size;
	VkDeviceSize offset;
} asVkAllocation_t;

/**
* @brief find the memory type for the device that fits the requirements given
* @return will return the type or -1 if no suitable type was found
* @param type bit
*/
int32_t asVkFindMemoryType(uint32_t typeBitsReq, VkMemoryPropertyFlags requiredProps);
/**
* @brief allocate vulkan memory
* @param mem allocated memory
* @param size amount of bytes to request for an allocation
* @param type the type of memory relative to the device, can be found with asVkFindMemoryType()
*/
void asVkAlloc(asVkAllocation_t* pMem, VkDeviceSize size, uint32_t type);
/**
* @brief free a vulkan allocation
*/
void asVkFree(asVkAllocation_t* pMem);

/**
* @brief try to map a region of memory to a pointer
*/
void asVkMapMemory(asVkAllocation_t mem, VkDeviceSize offset, VkDeviceSize size, void** ppData);
/**
* @brief unmap all the memory on an allocation
*/
void asVkUnmapMemory(asVkAllocation_t mem);
/**
* @brief Flush all memory related to an allocation
*/
void asVkFlushMemory(asVkAllocation_t mem);

/**
* @brief aquire a the next available graphics command buffer for the frame 
* Not reusable from frame to frame
* Use secondary command buffers for batching
*/
VkCommandBuffer asVkGetNextGraphicsCommandBuffer();
/**
* @brief aquire a the next available compute command buffer for the frame
* Not reusable from frame to frame
* Use secondary command buffers for batching
*/
VkCommandBuffer asVkGetNextComputeCommandBuffer();

/**
* @brief Get a VkImage from a texture at a slot
*/
VkImage asVkGetImageFromTexture(asTextureHandle_t hndl);
/**
* @brief Get a VkImageView from a texture at a slot
*/
VkImageView asVkGetViewFromTexture(asTextureHandle_t hndl);
/**
* @brief Get an asVkAllocation_t from a texture at a slot
*/
asVkAllocation_t asVkGetAllocFromTexture(asTextureHandle_t hndl);

/**
* @brief Get an VkBuffer from a buffer at a slot
*/
VkBuffer asVkGetBufferFromBuffer(asBufferHandle_t hndl);
/**
* @brief Get an VkBuffer from a buffer at a slot
*/
asVkAllocation_t asVkGetAllocFromBuffer(asBufferHandle_t hndl);

#endif

/*Shader Fx*/

#define AS_VK_MAX_PERMUTATION_ENTRIES 16

/*
* @brief a description for a pipeline permutation to be used with the fx system
*/
typedef struct
{
	asShaderFxProgramLookup_t requestedPrograms[6]; /**< Requested program lookups, parse until capacity or 0 is reached*/
	VkPipelineBindPoint pipelineType;
	union 
	{
		VkGraphicsPipelineCreateInfo* graphicsInfo;
		VkComputePipelineCreateInfo* computeInfo;
	};
} asVkFxPermutationEntry_t;

typedef struct
{
	asHash32_t requiredBucket; /**< Required bucket hash (UINT32_MAX is no requirements)*/
	asBlendMode requiredBlendMode; /**< Required blend mode (UINT32_MAX is no requirements)*/
	asFillMode requiredFillMode; /**< Required fill mode (UINT32_MAX is no requirements)*/
} asVkFxPermutationRequirements_t;

void asVkRegisterFxPipelinePermutation(const char* name, size_t nameSize, asVkFxPermutationRequirements_t req, asVkFxPermutationEntry_t desc);

#ifdef __cplusplus
}
#endif