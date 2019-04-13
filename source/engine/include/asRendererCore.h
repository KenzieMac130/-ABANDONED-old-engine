#pragma once

#include "asCommon.h"

/**
* @file
* @brief The mid level (above the API but below the ECS) core of the engine's renderer
*/

/**
* @brief Get the Main window as a SDL Window
*/
ASEXPORT struct SDL_Window* asGetMainWindowPtr();

/**
* @brief Initializes the window and renderer
* @warning The engine ignite should handle this for you
*/
ASEXPORT void asInitGfx(asAppInfo_t *pAppInfo, void* pCustomWindow);
/**
* @brief Shutdown the window and renderer
* @warning The engine shutdown should handle this for you
*/
ASEXPORT void asShutdownGfx();

/**
* @brief Types of textures
*/
typedef enum {
	AS_TEXTURETYPE_2D,
	AS_TEXTURETYPE_2DARRAY,
	AS_TEXTURETYPE_CUBE,
	AS_TEXTURETYPE_CUBEARRAY,
	AS_TEXTURETYPE_3D,
	AS_TEXTURETYPE_COUNT,
	AS_TEXTURETYPE_MAX = UINT32_MAX
} asTextureType; 

/**
* @brief Common image formats for rendering
*/
typedef enum {
	AS_TEXTUREFORMAT_RGBA8_UNORM,
	AS_TEXTUREFORMAT_RGBA16_UNORM,
	AS_TEXTUREFORMAT_RGBA16_SFLOAT,
	AS_TEXTUREFORMAT_R10G10B10A2_UNORM,
	AS_TEXTUREFORMAT_R8_UNORM,
	AS_TEXTUREFORMAT_R16_SFLOAT,
	AS_TEXTUREFORMAT_R32_SFLOAT,
	AS_TEXTUREFORMAT_R16G16_SFLOAT,
	AS_TEXTUREFORMAT_BC1_UNORM_BLOCK,
	AS_TEXTUREFORMAT_BC3_UNORM_BLOCK,
	AS_TEXTUREFORMAT_BC5_UNORM_BLOCK,
	AS_TEXTUREFORMAT_BC6H_UFLOAT_BLOCK,
	AS_TEXTUREFORMAT_BC7_UNORM_BLOCK,
	AS_TEXTUREFORMAT_DEPTH,
	AS_TEXTUREFORMAT_COUNT,
	AS_TEXTUREFORMAT_MAX = UINT32_MAX
} asTextureFormat;

/**
* @brief Texture usage flags
*/
typedef enum {
	AS_TEXTUREUSAGE_SAMPLED = 1 << 0,
	AS_TEXTUREUSAGE_RENDERTARGET = 1 << 1,
	AS_TEXTUREUSAGE_DEPTHBUFFER = 1 << 2,
	AS_TEXTUREUSAGE_TRANSFER_DST = 1 << 3,
	AS_TEXTUREUSAGE_TRANSFER_SRC = 1 << 4,
	AS_TEXTUREUSAGE_MAX = UINT32_MAX
} asTextureUsageFlags;

/**
* @brief Texture filtering
*/
typedef enum {
	AS_TEXTUREFILTER_LINEAR,
	AS_TEXTUREFILTER_NEAREST,
	AS_TEXTUREFILTER_MAX = UINT32_MAX
} asTextureFilterType;

/**
* @brief Texture wraping
*/
typedef enum {
	AS_TEXTUREWRAP_REPEAT,
	AS_TEXTUREWRAP_CLAMP,
	AS_TEXTUREWRAP_MIRROR,
	AS_TEXTUREWRAP_MAX = UINT32_MAX
} asTextureWrapType;

/**
* @brief The frequency at which a GPU resource will be updated
*/
typedef enum {
	AS_GPURESOURCEACCESS_IMMUTABLE, /**< Will only be uploaded to once if at all (static meshes, render targets, image textures)*/
	AS_GPURESOURCEACCESS_DYNAMIC, /**< May be updated throughout the lifetime of the resource, (deformation, materials, etc)*/
	AS_GPURESOURCEACCESS_STREAM, /**< Every frame the data might be different (movies, simulation geo, etc)*/
	AS_GPURESOURCEACCESS_MAX = UINT32_MAX
} asGpuResourceUploadType;

/**
* @brief Specifies a region of content in an image
*/
typedef struct {
	uint32_t bufferStart; /**< Starting position of this region in the buffer*/
	uint32_t offset[3]; /**< UVW Offset of the region in pixels*/
	uint32_t extent[3]; /**< UVW Extent of the region in pixels*/
	uint32_t mipLevel; /**< Which mip level this region belongs to*/
	uint32_t layer; /**< Which layer this region belongs to*/
	uint32_t layerCount; /**< How many layers this region has in it*/
} asTextureContentRegion_t;

/**
* @brief Description for a texture resource
*/
typedef struct {
	asTextureType type; /**< What type of texture is this*/
	asGpuResourceUploadType cpuAccess; /**< CPU access info*/
	asTextureFormat format; /**< Color format of the texture*/
	int32_t usageFlags; /**< Usage of the texture (default assumes its sampled in a shader)*/
	uint32_t width; /**< Width in pixels*/
	uint32_t height; /**< Height in pixels*/
	uint32_t depth; /**< Depth in pixels (or amount of layers in an array)*/
	uint32_t mips; /**< Number of mip levels*/
	uint32_t maxAnisotropy; /**< Maximum anisotropy (by default it sets it to 1)*/
	asTextureWrapType uWrap; /**< How to wrap the texture horizontally (default repeats)*/
	asTextureWrapType vWrap; /**< How to wrap the texture vertically (default repeats)*/
	asTextureWrapType wWrap; /**< How to wrap the texture depth wise (only aplicable to asTextureType::AS_TEXTURETYPE_3D, default repeats)*/
	asTextureFilterType minFilter; /**< Minification filter*/
	asTextureFilterType magFilter; /**< Maxification filter*/
	float minLod; /**< Minimum mipmap LOD*/
	float maxLod; /**< Maximum mipmap LOD*/
	size_t initialContentsBufferSize; /**< The size of the data in the buffer to be uploaded*/
	void* pInitialContentsBuffer; /**< Pointer to the initial contents of the texture (if NULL nothing will be uploaded)*/
	size_t initialContentsRegionCount; /**< The amount of regions to copy into (if 0 the buffer will attempt to copy into the first slice)*/
	asTextureContentRegion_t *pInitialContentsRegions; /**< Used to map buffer data to parts of the texture image (if NULL same above)*/
	const char* pDebugLabel; /**< Debug name of the texture that should appear in debuggers*/
} asTextureDesc_t;

/**
* @brief Set the defaults for a texture
*/
ASEXPORT asTextureDesc_t asTextureDesc_Init();

/**
* @brief Calculate the pitch of a texture
*/
ASEXPORT uint32_t asTextureCalcPitch(asTextureFormat format, uint32_t width);

/**
* @brief Native size of a depth buffer
*/
ASEXPORT uint32_t asGetDepthFormatSize(asTextureFormat preference);

/**
* @brief Hanlde to a texture resource
*/
typedef asHandle_t asTextureHandle_t;

/**
* @brief API Independent mechanism for creating texture resources
* @warning not guaranteed to be threadsafe
*/
ASEXPORT asTextureHandle_t asCreateTexture(asTextureDesc_t *pDesc);

/**
* @brief API Independent mechanism for releasing texture resources
* @warning not guaranteed to be immideate/threadsafe
*/
ASEXPORT void asReleaseTexture(asTextureHandle_t hndl);

/**
* @brief Maximum number of textures
*/
#define AS_MAX_TEXTURES 512