#pragma once

#include "../common/asCommon.h"
#include "../asResource.h"
#ifdef __cplusplus
extern "C" {
#endif 

#define AS_MAX_INFLIGHT 2

/**
* @file
* @brief The mid level (above the API but below the ECS/postfx/etc) core of the engine's renderer
* Its job is not to abstract/encapsulate every aspect of rendering APIs but rather provide only meaninful and useful utilities
* to tasks such as texture/buffer resource management/interfacing with programmable shaders and materials
* each subsystem of the renderer is responsible for implimenting rendering algorithms (often using native calls) for each backend/API
*/
typedef enum {
	AS_GFXAPI_NULL,
	AS_GFXAPI_VULKAN,
	AS_GFXAPI_OPENGL,
	AS_GFXAPI_DIRECTX,
	AS_GFXAPI_METAL,
	AS_GFXAPI_MAX = UINT32_MAX
} asGfxAPIs;

#if ASTRENGINE_VK
/*Current Graphics API*/
#define AS_GFXAPI AS_GFXAPI_VULKAN
/*Major API Version (example: (dx)12, (vk)1, (gl)45, etc)*/
#define AS_GFXAPI_VERSION 1
#endif

/**
* @brief Get the Main window as a SDL Window
*/
ASEXPORT struct SDL_Window* asGetMainWindowPtr();

/**
* @brief Initializes the window and renderer
* @warning The engine ignite should handle this for you
*/
ASEXPORT void asInitGfx(asLinearMemoryAllocator_t* pLinearAllocator, asAppInfo_t *pAppInfo, void* pCustomWindow);
/**
* @brief Render a single frame
* @warning The engine update should handle this for you
*/
ASEXPORT void asGfxRenderFrame();

/**
* @brief Trigger a window resize event, the renderer will attempt to adjust to it
*/
ASEXPORT void asGfxTriggerResizeEvent();

/**
* @brief Tells the program to skip frame drawing
*/
ASEXPORT void asGfxSetDrawSkip(bool skip);
/**
* @brief Shutdown the window and renderer
* @warning The engine shutdown should handle this for you
*/
ASEXPORT void asShutdownGfx();

/*Textures*/

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
* @brief Common image color formats for rendering
*/
typedef enum {
	AS_COLORFORMAT_DEPTH,
	AS_COLORFORMAT_DEPTH_LP,
	AS_COLORFORMAT_DEPTH_STENCIL,
	AS_COLORFORMAT_RGBA8_UNORM,
	AS_COLORFORMAT_RGBA16_UNORM,
	AS_COLORFORMAT_RGBA16_SFLOAT,
	AS_COLORFORMAT_RGBA32_SFLOAT,
	AS_COLORFORMAT_R10G10B10A2_UNORM,
	AS_COLORFORMAT_B10G11R11_UFLOAT,
	AS_COLORFORMAT_R8_UNORM,
	AS_COLORFORMAT_R16_SFLOAT,
	AS_COLORFORMAT_R32_SFLOAT,
	AS_COLORFORMAT_RG16_SFLOAT,
	AS_COLORFORMAT_RG32_SFLOAT,
	AS_COLORFORMAT_RGB16_SFLOAT,
	AS_COLORFORMAT_RGB32_SFLOAT,
	AS_COLORFORMAT_RGBA32_UINT,
	AS_COLORFORMAT_BC1_UNORM_BLOCK,
	AS_COLORFORMAT_BC3_UNORM_BLOCK,
	AS_COLORFORMAT_BC5_UNORM_BLOCK,
	AS_COLORFORMAT_BC6H_UFLOAT_BLOCK,
	AS_COLORFORMAT_BC7_UNORM_BLOCK,
	AS_COLORFORMAT_COUNT,
	AS_COLORFORMAT_MAX = UINT32_MAX
} asColorFormat;

typedef enum {
	AS_COLORCHANNEL_R = 1 << 0,
	AS_COLORCHANNEL_G = 1 << 1,
	AS_COLORCHANNEL_B = 1 << 2,
	AS_COLORCHANNEL_A = 1 << 3,
	AS_COLORCHANNEL_COLOR =
	AS_COLORCHANNEL_R |
	AS_COLORCHANNEL_G |
	AS_COLORCHANNEL_B,
	AS_COLORCHANNEL_ALL =
	AS_COLORCHANNEL_COLOR |
	AS_COLORCHANNEL_A,
	AS_COLORCHANNEL_MAX = UINT32_MAX
} asColorChannelsFlags;

/**
* @brief Texture usage flags
*/
typedef enum {
	AS_TEXTUREUSAGE_TRANSFER_DST = 1 << 0,
	AS_TEXTUREUSAGE_TRANSFER_SRC = 1 << 1,
	AS_TEXTUREUSAGE_SAMPLED = 1 << 2,
	AS_TEXTUREUSAGE_RENDERTARGET = 1 << 3,
	AS_TEXTUREUSAGE_DEPTHBUFFER = 1 << 4,
	AS_TEXTUREUSAGE_MAX = UINT32_MAX
} asTextureUsageFlags;

/**
* @brief The way in which the cpu will access the resource and its allocation behavior
*/
typedef enum {
	AS_GPURESOURCEACCESS_DEVICE, /**< Staging uploads are required so the resource will be updated infrequently but faster for GPU*/
	AS_GPURESOURCEACCESS_STAGING, /**< Use this for staging uploads, not buffered for rendering without artifacts*/
	AS_GPURESOURCEACCESS_STREAM, /**< Every frame the data might be different and the host can access this directly*/
	AS_GPURESOURCEACCESS_COUNT,
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
	asColorFormat format; /**< Color format of the texture*/
	int32_t usageFlags; /**< Usage of the texture (default assumes its sampled in a shader)*/
	uint32_t width; /**< Width in pixels*/
	uint32_t height; /**< Height in pixels*/
	uint32_t depth; /**< Depth in pixels (or amount of layers in an array)*/
	uint32_t mips; /**< Number of mip levels*/
	size_t initialContentsBufferSize; /**< The size of the data in the buffer to be uploaded*/
	const void* pInitialContentsBuffer; /**< Pointer to the initial contents of the texture (if NULL nothing will be uploaded)*/
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
ASEXPORT uint32_t asTextureCalcPitch(asColorFormat format, uint32_t width);

/**
* @brief Native size of a depth buffer
*/
ASEXPORT uint32_t asGetDepthFormatSize(asColorFormat preference);

/**
* @brief Hanlde to a texture resource
*/
typedef asHandle_t asTextureHandle_t;

/**
* @brief API independent mechanism for creating texture resources
* @warning not guaranteed to be threadsafe
*/
ASEXPORT asTextureHandle_t asCreateTexture(asTextureDesc_t *pDesc);

/**
* @brief API independent mechanism for releasing texture resources
* @warning not guaranteed to be immideate/threadsafe
*/
ASEXPORT void asReleaseTexture(asTextureHandle_t hndl);

/**
* @brief Maximum number of textures
*/
#define AS_MAX_TEXTURES 512

/*Buffers*/

/**
* @brief Texture usage flags
*/
typedef enum {
	AS_BUFFERUSAGE_TRANSFER_DST = 1 << 0,
	AS_BUFFERUSAGE_TRANSFER_SRC = 1 << 1,
	AS_BUFFERUSAGE_INDEX = 1 << 2,
	AS_BUFFERUSAGE_VERTEX = 1 << 3,
	AS_BUFFERUSAGE_INDIRECT = 1 << 4,
	AS_BUFFERUSAGE_UNIFORM = 1 << 5,
	AS_BUFFERUSAGE_STORAGE = 1 << 6,
	AS_BUFFERUSAGE_MAX = UINT32_MAX
} asBufferUsageFlags;

/**
* @brief Description for a buffer resource
*/
typedef struct {
	asGpuResourceUploadType cpuAccess; /**< CPU access info*/
	int32_t usageFlags; /**< Usage of the buffer*/
	uint32_t bufferSize; /**< Size of the buffer in bytes*/
	size_t initialContentsBufferSize; /**< The size of the data in the buffer to be uploaded*/
	void* pInitialContentsBuffer; /**< Pointer to the initial contents of the buffer (if NULL nothing will be uploaded)*/
	const char* pDebugLabel; /**< Debug name of the texture that should appear in debuggers*/
} asBufferDesc_t;

/**
* @brief Set the defaults for a buffer
*/
ASEXPORT asBufferDesc_t asBufferDesc_Init();

/**
* @brief Hanlde to a buffer resource
*/
typedef asHandle_t asBufferHandle_t;

/**
* @brief API independent mechanism for creating buffer resources
* @warning not guaranteed to be threadsafe
*/
ASEXPORT asBufferHandle_t asCreateBuffer(asBufferDesc_t *pDesc);

/**
* @brief API independent mechanism for releasing buffer resources
* @warning not guaranteed to be immideate/threadsafe
*/
ASEXPORT void asReleaseBuffer(asBufferHandle_t hndl);

/**
* @brief Maximum number of buffers
*/
#define AS_MAX_BUFFERS 1024

/*Shader FX*/

/**
* @brief Maximum number of shader fx
*/
#define AS_MAX_SHADERFX 64

/**
* @brief shader stages
*/
typedef enum {
	AS_SHADERSTAGE_VERTEX, /**< Vertex shader*/
	AS_SHADERSTAGE_TESS_CONTROL, /**< Tessellation Hull or Control shader*/
	AS_SHADERSTAGE_TESS_EVALUATION, /**< Tessellation Domain or Evaluation shader*/
	AS_SHADERSTAGE_GEOMETRY, /**< Geometry shader*/
	AS_SHADERSTAGE_FRAGMENT, /**< Fragment shader (sometimes referred to as pixel shaders)*/
	AS_SHADERSTAGE_COMPUTE, /**< Compute shaders*/
	AS_SHADERSTAGE_COUNT,
	AS_SHADERSTAGE_MAX = UINT32_MAX
} asShaderStage;

/**
* @brief shader code lookup
*/
typedef struct {
	asHash32_t nameHash; /**< Name of the pipeline permutation*/
	asShaderStage stage; /**< Stage this shader belongs to*/
} asShaderFxProgramLookup_t;

/**
* @brief Describes a shader program
* main is always assumed to be the entry point of the code
*/
typedef struct {
	uint32_t programByteStart; /**< Shader code/bytecode for the current API (Vulkan: SPIR-V, OpenGL: GLSL, DirectX: HLSL, ect...)*/
	uint32_t programByteCount; /**< Amount of bytes in asShaderCodeDesc_t::pProgramBytes*/
} asShaderFxProgramDesc_t;

/**
* @brief Fill modes
*/
typedef enum {
	AS_FILL_FULL,
	AS_FILL_WIRE,
	AS_FILL_POINTS,
	AS_FILL_COUNT,
	AS_FILL_MAX = UINT32_MAX
} asFillMode;

/**
* @brief Cull modes
*/
typedef enum {
	AS_CULL_BACK,
	AS_CULL_FRONT,
	AS_CULL_NONE,
	AS_CULL_COUNT,
	AS_CULL_MAX = UINT32_MAX
} asCullMode;

/**
* @brief High level blend modes
*/
typedef enum {
	AS_BLEND_NONE,
	AS_BLEND_MIX,
	AS_BLEND_ADD,
	AS_BLEND_MAX = UINT32_MAX
} asBlendMode;

/**
* @brief shader fx property types
*/
typedef enum {
	AS_SHADERFXPROP_SCALAR,
	AS_SHADERFXPROP_VECTOR4,
	AS_SHADERFXPROP_INTEGER,
	AS_SHADERFXPROP_TEX2D,
	AS_SHADERFXPROP_TEX2DARRAY,
	AS_SHADERFXPROP_TEXCUBE,
	AS_SHADERFXPROP_TEXCUBEARRAY,
	AS_SHADERFXPROP_TEX3D,
	AS_SHADERFXPROP_MAX = UINT8_MAX
} asShaderFxPropType;

/**
* @brief shader material property lookup
*/
typedef struct {
	asHash32_t nameHash;
	asShaderFxPropType type;
} asShaderFxPropLookup_t;

/**
* @brief shader material property defaults
*/
typedef struct {
	float values[4];
} asShaderFxTextureDefault_t;

typedef enum {
	AS_SHADERFXTEXDEFAULT_BLACK,
	AS_SHADERFXTEXDEFAULT_WHITE,
	AS_SHADERFXTEXDEFAULT_GREY,
	AS_SHADERFXTEXDEFAULT_NORMAL,
	AS_SHADERFXTEXDEFAULT_ALPHA,
	AS_SHADERFXTEXDEFAULT_MAX = UINT8_MAX
} asShaderFxTextureDefault;

#define AS_MATERIAL_BUFFER_SIZE 256

#define AS_SHADERFX_VERSION 1

/**
* @brief Description for a shader effect
*/
typedef struct {
	uint32_t flags; /**< Flags for future use*/
	asHash32_t bucket; /**< Requested rendering bucket for special rendering inputs (eg, transparent surfaces)*/
	uint32_t tessCtrlPoints; /**< Tessellation control points*/
	float fillSize; /**< Width for non-solid lines and points*/
	asFillMode fillMode; /**< Polygon fill modes*/
	asCullMode cullMode; /**< Which side of the face if any is culled*/
	asBlendMode blendMode; /**< High level blend modes for the surface*/
	uint32_t reserved[5]; /**< Reserved for future use*/

	uint32_t propCount; /**< Amount of asShaderDesc_t::pFxProps*/
	uint32_t programCount; /**< Amount of asShaderDesc_t::pProgramDescs*/
	uint32_t shaderCodeSize; /**< Size of asShaderFxDesc_t::pShaderCode*/
	uint32_t propBufferSize; /**< The size of the buffer data required for fx properties*/

	asShaderFxPropLookup_t *pPropLookup; /**< Lookup value for a property*/
	uint16_t *pPropOffset; /**< This is the byte offset for the propbuffer*/

	asShaderFxProgramLookup_t *pProgramLookup; /**< Lookup value of each shader program*/
	asShaderFxProgramDesc_t *pProgramDescs; /**< Programm contents offsets*/
	unsigned char* pShaderCode; /**<  Shader code/bytecode for the current API (Vulkan: SPIR-V, OpenGL: GLSL, DirectX: HLSL, ect...)*/

	unsigned char* pPropBufferDefault; /**< Default fx prop buffer (material values)*/
} asShaderFxDesc_t;

/**
* @brief Set the defaults for a shader
*/
ASEXPORT asShaderFxDesc_t asShaderFxDesc_Init();

/**
* @brief Save the fx to file
*/
ASEXPORT int32_t asShaderFxDesc_SaveToFile(const char* fileName, asShaderFxDesc_t *fx, asGfxAPIs api, uint32_t apiVersion);

/**
* @brief Hanlde to an internal representation of shaderfx 
*/
typedef asHandle_t asShaderFxHandle_t;

/**
* @brief Create the shaderfx from a description
*/
ASEXPORT asShaderFxHandle_t asShaderFx_FromDesc(asShaderFxDesc_t* desc, const char* name, int32_t nameLength);

/**
* @brief Create the shaderfx from a resource
*/
ASEXPORT asShaderFxHandle_t asShaderFx_FromResource(asResourceFileID_t file);

/*Material System*/

/**
* @brief Description for a material
*/
typedef struct {
	asResourceFileID_t fxFile; /**< ID for the fx associated with this material*/
	uint32_t propCount; /**< Amount of asMaterialDesc_t::pProps*/
	uint32_t textureFileCount; /**< Amount of asMaterialDesc_t::pProps*/
	uint32_t propBufferSize; /**< The size of asMaterialDesc_t::pPropBuffer*/

	asResourceFileID_t *pTextureFiles; /**< IDs for the texture files associated with the material*/
	asShaderFxPropLookup_t *pPropLookup; /**< Lookup value for a property*/
	uint16_t *pPropOffset; /**< This is the byte offset for the propbuffer*/

	unsigned char* pPropBuffer; /**< Contains the material values*/
} asMaterialDesc_t;

#ifdef __cplusplus
}
#endif