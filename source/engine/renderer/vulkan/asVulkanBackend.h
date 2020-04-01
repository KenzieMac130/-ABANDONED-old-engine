#ifndef _ASVULKANBACKEND_H_
#define _ASVULKANBACKEND_H_

#include "../../common/asCommon.h"
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
void asVkInit(asAppInfo_t *pAppInfo, asCfgFile_t* pConfig);

/**
* @brief start shutdown vulkan backend
* destroys every vulkan resource for a clean shutdown
*/
void asVkInitShutdown();
/**
* @brief finalize shutdown vulkan backend
* destroys every vulkan resource for a clean shutdown
*/
void asVkFinalShutdown();

/**
* @brief initialize the frame
*/
void asVkInitFrame();

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

VkFormat asVkConvertToNativePixelFormat(asColorFormat format);

VkShaderStageFlagBits asVkConvertToNativeStage(asShaderStage stage);

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
* @brief Get an asVkAllocation_t from a buffer at a slot
*/
asVkAllocation_t asVkGetAllocFromBuffer(asBufferHandle_t hndl);

/**
* @brief Renderpass for simple drawing (guizmos, UI, early renderer)
*/
VkRenderPass asVkGetSimpleDrawingRenderpass();
/**
* @brief Framebuffer for simple drawing (guizmos, UI, early renderer)
*/
VkFramebuffer asVkGetSimpleDrawingFramebuffer();

/**
* @brief Sampler for simple texturing
*/
VkSampler* asVkGetSimpleSamplerPtr(bool interpolate);

/*Pipeline Creation Helpers (Because Vulkan has a lot of paperwork)*/

/*Fill out color blend state*/
#define AS_VK_INIT_HELPER_PIPE_COLOR_BLEND_STATE(_attachmentCount, _pAttachments)\
(VkPipelineColorBlendStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,\
.logicOpEnable = VK_FALSE,\
.logicOp = VK_LOGIC_OP_COPY,\
.blendConstants = {0.0f,0.0f,0.0f,0.0f},\
.attachmentCount = _attachmentCount,\
.pAttachments = _pAttachments\
}

/*Fill out color blend attachment state with basic blending*/
#define AS_VK_INIT_HELPER_ATTACHMENT_BLEND_MIX(_vkbEnable)\
(VkPipelineColorBlendAttachmentState){\
.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,\
.blendEnable = _vkbEnable,\
.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,\
.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,\
.colorBlendOp  = VK_BLEND_OP_ADD,\
.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,\
.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,\
.alphaBlendOp  = VK_BLEND_OP_ADD,\
}

/*Fill out depth stencil state*/
#define AS_VK_INIT_HELPER_PIPE_DEPTH_STENCIL_STATE(_vkbDepthTest, _vkbDepthWrite, _depthCompareOp, _vkbDepthBound, _minDepthBound, _maxDepthBound)\
(VkPipelineDepthStencilStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,\
.depthTestEnable = _vkbDepthTest,\
.depthWriteEnable = _vkbDepthWrite,\
.depthCompareOp = _depthCompareOp,\
.depthBoundsTestEnable = _vkbDepthBound,\
.stencilTestEnable = VK_FALSE,\
.minDepthBounds = _minDepthBound,\
.maxDepthBounds = _maxDepthBound\
}

/*Fill out rasterizer state*/
#define AS_VK_INIT_HELPER_PIPE_RASTERIZATION_STATE(_polygonMode, _cullMode, _fontFace, _vkbDepthBiasEnable, _depthBiasConstant, _depthBiasSlope, _depthBiasClamp, _lineWidth)\
(VkPipelineRasterizationStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,\
.rasterizerDiscardEnable = VK_FALSE,\
.polygonMode = _polygonMode,\
.cullMode = _cullMode,\
.frontFace = _fontFace,\
.depthBiasEnable = _vkbDepthBiasEnable,\
.depthBiasConstantFactor = _depthBiasConstant,\
.depthBiasSlopeFactor = _depthBiasSlope,\
.depthBiasClamp = _depthBiasClamp,\
.lineWidth = _lineWidth,\
}

/*Fill out input assembler state*/
#define AS_VK_INIT_HELPER_PIPE_INPUT_ASSEMBLER_STATE(_topology, _vkbRestart)\
(VkPipelineInputAssemblyStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,\
.primitiveRestartEnable = _vkbRestart,\
.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST\
}

/*Fill out MSAA state as disabled*/
#define AS_VK_INIT_HELPER_PIPE_MSAA_STATE_NONE()\
(VkPipelineMultisampleStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,\
.sampleShadingEnable  = VK_FALSE,\
.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,\
}

/*Fill out tessellation state*/
#define AS_VK_INIT_HELPER_PIPE_TESS_STATE(_points)\
(VkPipelineTessellationStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,\
.patchControlPoints = _points\
}

/*Fill out Viewport/Scissors state to include everyhing (will be dynamic anyways)*/
#define AS_VK_INIT_HELPER_PIPE_VIEWPORT_STATE_EVERYTHING(_viewX, _viewY)\
(VkPipelineViewportStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,\
.viewportCount = 1,\
.pViewports = &(VkViewport){\
	.x = 0.0f, .y = 0.0f,\
	.width = (float)_viewX, .height = (float)_viewY,\
	.minDepth = 0.0f, .maxDepth = 1.0f},\
.scissorCount = 1,\
.pScissors = &(VkRect2D){\
	.offset = {0,0},\
	.extent = {_viewX, _viewY}}\
}

/*Fill out dynamics state*/
#define AS_VK_INIT_HELPER_PIPE_DYNAMIC_STATE(_dynamicCount, _pDynamics)\
(VkPipelineDynamicStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,\
.dynamicStateCount = _dynamicCount,\
.pDynamicStates = _pDynamics\
}

/*Fill out dynamics state to default settings*/
#define AS_VK_INIT_HELPER_PIPE_VERTEX_STATE(_bindingCount, _pBindings, _attributeCount, _pAttributes)\
(VkPipelineVertexInputStateCreateInfo){\
.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,\
.vertexBindingDescriptionCount = _bindingCount,\
.pVertexBindingDescriptions = _pBindings,\
.vertexAttributeDescriptionCount = _attributeCount,\
.pVertexAttributeDescriptions = _pAttributes,\
}

/*Fill out vertex binding*/
#define AS_VK_INIT_HELPER_VERTEX_BINDING(_binding, _stride, _inputRate)\
(VkVertexInputBindingDescription) {\
.binding = _binding,\
.stride = _stride,\
.inputRate = _inputRate\
}

/*Fill out vertex attributes*/
#define AS_VK_INIT_HELPER_VERTEX_ATTRIBUTE(_location, _binding, _format, _offset)\
(VkVertexInputAttributeDescription) {\
.location = _location,\
.binding = _binding,\
.format = _format,\
.offset = _offset\
}


#endif

#ifdef __cplusplus
}
#endif
#endif