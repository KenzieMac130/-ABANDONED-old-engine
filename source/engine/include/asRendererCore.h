#pragma once

#include "asCommon.h"

/**
* @file
* @brief The mid level (above the API but below the ECS/postfx/etc) core of the engine's renderer
* Its job is not to encapsulate every aspect of rendering API but rather provide only meaninful and useful abstractions
* to tasks such as texture/buffer resource management/interfacing with programmable shaders and materials
* each subsystem of the renderer is responsible for implimenting rendering algorithms (often using native calls) for each backend/API
*/

typedef enum {
	AS_GFXAPI_NULL,
	AS_GFXAPI_VULKAN,
	AS_GFXAPI_MAX = UINT32_MAX
} asGfxAPIs;

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

/*Texture Stuff*/

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

/*Buffer Stuff*/

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

/*Shader FX Stuff*/

/**
* @brief Sampler filtering
*/
typedef enum {
	AS_SAMPLERFILTER_LINEAR,
	AS_SAMPLERFILTER_NEAREST,
	AS_SAMPLERFILTER_COUNT,
	AS_SAMPLERFILTER_MAX = UINT32_MAX
} asSamplerFilterType;

/**
* @brief Sampler wraping
*/
typedef enum {
	AS_SAMPLERWRAP_REPEAT,
	AS_SAMPLERWRAP_CLAMP,
	AS_SAMPLERWRAP_MIRROR,
	AS_SAMPLERWRAP_COUNT,
	AS_SAMPLERWRAP_MAX = UINT32_MAX
} asSamplerWrapType;

/**
* @brief Description for a sampler
*/
typedef struct {
	asSamplerWrapType uWrap; /**< How to wrap the texture horizontally (default repeats)*/
	asSamplerWrapType vWrap; /**< How to wrap the texture vertically (default repeats)*/
	asSamplerWrapType wWrap; /**< How to wrap the texture depth wise (only aplicable to asTextureType::AS_TEXTURETYPE_3D, default repeats)*/
	asSamplerFilterType minFilter; /**< Minification filter*/
	asSamplerFilterType magFilter; /**< Maxification filter*/
	float minLod; /**< Minimum mipmap LOD*/
	float maxLod; /**< Maximum mipmap LOD*/
	uint32_t anisotropy; /**< Anisotropy (by default it sets it to 1)*/
} asSamplerDesc_t;

/**
* @brief shader resource types
*/
typedef enum {
	AS_SHADERBINDING_UBO, 
	AS_SHADERBINDING_SBO,
	AS_SHADERBINDING_SAMPLER,
	AS_SHADERBINDING_TEX2D,
	AS_SHADERBINDING_TEX2DARRAY,
	AS_SHADERBINDING_TEXCUBE,
	AS_SHADERBINDING_TEXCUBEARRAY,
	AS_SHADERBINDING_TEX3D,
	AS_SHADERBINDING_MAX = UINT32_MAX
} asShaderFxBindingType;

/**
* @brief Description for a shader resource binding
*/
typedef struct {
	uint32_t slot;
	asShaderFxBindingType type;
	asHash64_t nameHash;
} asShaderFxBindingDesc_t;

/**
* @brief Description for shader inputs/outputs
*/
typedef struct {
	uint32_t slot;
	asColorFormat type;
	asHash64_t nameHash;
} asShaderFxIODesc_t;

/**
* @brief shader variable types (We only care about some of the common ones)
*/
typedef enum {
	AS_SHADERVARTYPE_FLOAT,
	AS_SHADERVARTYPE_FVECTOR2,
	AS_SHADERVARTYPE_FVECTOR3,
	AS_SHADERVARTYPE_FVECTOR4,
	AS_SHADERVARTYPE_FMATRIX2X2,
	AS_SHADERVARTYPE_FMATRIX3X3,
	AS_SHADERVARTYPE_FMATRIX4X4,
	AS_SHADERVARTYPE_INTEGER,
	AS_SHADERVARTYPE_IVECTOR2,
	AS_SHADERVARTYPE_IVECTOR3,
	AS_SHADERVARTYPE_IVECTOR4,
	AS_SHADERVARTYPE_COUNT,
	AS_SHADERVARTYPE_MAX = UINT32_MAX
} asShaderFxVarType;

/**
* @brief Description for buffer members
*/
typedef struct {
	asShaderFxVarType type;
	asHash64_t nameHash;
} asShaderFxBufferMemberDesc_t;

/**
* @brief Description for buffer layouts
*/
typedef struct {
	asHash64_t nameHash;
	uint32_t memberCount;
	asShaderFxBufferMemberDesc_t *pMembers;
} asShaderFxBufferLayoutDesc_t;

/**
* @brief shader stages
*/
typedef enum {
	AS_SHADERSTAGE_VERTEX, /**< Vertex shader*/
	AS_SHADERSTAGE_DOMAIN, /**< Tessellation Domain or Control shader*/
	AS_SHADERSTAGE_HULL, /**< Tessellation Hull or Evaluation shader*/
	AS_SHADERSTAGE_GEOMETRY, /**< Geometry shader*/
	AS_SHADERSTAGE_FRAGMENT, /**< Fragment shader (sometimes referred to as pixel shaders)*/
	AS_SHADERSTAGE_COMPUTE, /**< Compute shaders*/
	AS_SHADERSTAGE_COUNT,
	AS_SHADERSTAGE_MAX = UINT32_MAX
} asShaderStage;

/**
* @brief Describes a shader program
* main is always assumed to be the entry point of the code
*/
typedef struct {
	asHash64_t permutationHash; /**< The hash of the shader permutation (different shaders can have different techniques)*/
	asShaderStage stage; /**< Stage this shader belongs to*/
	size_t programByteCount; /**< Amount of bytes in asShaderCodeDesc_t::pProgramBytes*/
	uint8_t* pProgramBytes; /**< Shader code/bytecode for the current API (Vulkan: SPIR-V, OpenGL: GLSL, DirectX: HLSL, ect...)*/
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

typedef enum {
	AS_CULL_BACK,
	AS_CULL_FRONT,
	AS_CULL_NONE,
	AS_CULL_COUNT,
	AS_CULL_MAX = UINT32_MAX
} asCullMode;

typedef enum {
	AS_DEPTH_READWRITE_FRONT,
	AS_DEPTH_READWRITE_BEHIND,
	AS_DEPTH_READ_FRONT,
	AS_DEPTH_READ_BEHIND,
	AS_DEPTH_WRITE,
	AS_DEPTH_IGNORE,
	AS_DEPTH_COUNT,
	AS_DEPTH_MAX = UINT32_MAX
} asDepthTestMode;

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

typedef enum {
	AS_BLENDFUNC_ADD,
	AS_BLENDFUNC_SUB,
	AS_BLENDFUNC_REVERSESUB,
	AS_BLENDFUNC_MINIMUM,
	AS_BLENDFUNC_MAXIMUM,
	AS_BLENDFUNC_COUNT,
	AS_BLENDFUNC_MAX = UINT32_MAX
} asBlendFuncs;

typedef enum {
	AS_BLENDPARAMS_ONE,
	AS_BLENDPARAMS_ZERO,
	AS_BLENDPARAMS_SRCCOLOR,
	AS_BLENDPARAMS_DSTCOLOR,
	AS_BLENDPARAMS_ONEMINUS_SRCCOLOR,
	AS_BLENDPARAMS_ONEMINUS_DSTCOLOR,
	AS_BLENDPARAMS_SRCALPHA,
	AS_BLENDPARAMS_DSTALPHA,
	AS_BLENDPARAMS_ONEMINUS_SRCALPHA,
	AS_BLENDPARAMS_ONEMINUS_DSTALPHA,
	AS_BLENDPARAMS_COUNT,
	AS_BLENDPARAMS_MAX = UINT32_MAX
} asBlendParams;

typedef struct
{
	bool enabled;
	int32_t channelsEnabledFlags;
	asBlendFuncs colorFunc;
	asBlendParams srcColorParam;
	asBlendParams dstColorParam;
	asBlendFuncs alphaFunc;
	asBlendParams srcAlphaParam;
	asBlendParams dstAlphaParam;
} asBlendDesc_t;

#define AS_MAX_PIXELOUTPUTS 8

/**
* @brief Describes fixed function inputs for a shader
*/
typedef struct {
	uint32_t tessCtrlPoints; /** Tessellation control points*/
	float fillWidth; /**< For non-solid lines*/
	asFillMode fillMode; /**< Polygon fill modes*/
	asCullMode cullMode; /**< Which side of the face if any is culled*/
	asDepthTestMode depthMode; /**< Depth testing*/
	asBlendDesc_t colorBlends[AS_MAX_PIXELOUTPUTS]; /**< Color blending per pixel output*/
} asShaderFxFixedFunctionsDesc_t;

/**
* @brief shader material defaults
*/
typedef struct {
	asHash64_t nameHash;
	float values[4];
} asShaderFxMaterialDefault_t;

/**
* @brief Description for a shader effect
*/
typedef struct {
	asHash64_t nameHash; /**< A hash of the shader's name*/
	asGfxAPIs api; /**< Graphics API for the current shader*/
	asShaderFxFixedFunctionsDesc_t fixedFunctions; /**< Fixed functions for the shader*/
	uint32_t bindingCount; /**< Amount of asShaderDesc_t::pBindings*/
	asShaderFxBindingDesc_t *pBindings; /**< Shader resources to be bound in the shader*/
	uint32_t vertexInputCount; /**< Amount of asShaderDesc_t::pVertexInputs*/
	asShaderFxIODesc_t *pVertexInputs; /**< Vertex layout in the shader*/
	uint32_t pixelOutputCount; /**< Amount of asShaderDesc_t::pPixelOutputs, maximum of AS_MAX_SHADEROUTPUTS*/
	asShaderFxIODesc_t pixelOutputs[AS_MAX_PIXELOUTPUTS]; /**< Pixel outputs of the shader*/
	uint32_t bufferDescCount; /**< Amount of asShaderDesc_t::pBufferDescs*/
	asShaderFxBufferLayoutDesc_t *pBufferDescs; /**< Description of the buffer contents*/
	uint32_t samplerDescCount; /**< Amount of asShaderDesc_t::pSamplerDescs*/
	asSamplerDesc_t *pSamplerDescs; /**< Description of the samplers used in the shader*/
	uint32_t shaderProgramCount; /**< Amount of asShaderDesc_t::pShaderPrograms*/
	asShaderFxProgramDesc_t *pShaderPrograms; /**< Shader programs*/
	uint32_t materialDefaultCount; /**< Amount of asShaderDesc_t::pMaterialDefaults*/
	asShaderFxMaterialDefault_t *pMaterialDefaults; /**< Default material properties*/
} asShaderFxDesc_t;

/**
* @brief Set the defaults for a shader
*/
ASEXPORT asShaderFxDesc_t asShaderFxDesc_Init();