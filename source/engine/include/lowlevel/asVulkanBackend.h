#pragma once

#include "../asCommon.h"
#if ASTRENGINE_VK
#include <vulkan/vulkan.h>
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

#define AS_VK_MAX_INFLIGHT 2

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
void asVkInit(asAppInfo_t *pAppInfo, asCfgFile_t* pConfig);
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
void asVkWindowResize();

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
#endif