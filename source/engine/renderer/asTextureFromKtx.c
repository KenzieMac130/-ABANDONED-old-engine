#include "tiny_ktx/tinyktx.h"
#include "asTextureFromKtx.h"

struct ktxFromBlobData
{
	int64_t pos;
	void* pKtxBlob;
	size_t blobSize;
	asResults error;
};

void ktxError(void* user, char const* msg)
{
	struct ktxFromBlobData* pData = (struct ktxFromBlobData*)user;
	asDebugError("KTX Error: %s", msg);
	pData->error = AS_FAILURE_UNKNOWN;
}

int64_t ktxTellBlob(void* user)
{
	struct ktxFromBlobData* pData = (struct ktxFromBlobData*)user;
	return pData->pos;
}

bool ktxSeekBlob(void* user, int64_t offset)
{
	struct ktxFromBlobData* pData = (struct ktxFromBlobData*)user;
	pData->pos = offset < pData->blobSize ? offset : pData->blobSize;
	return (bool)(pData->pos < pData->blobSize);
}

void* ktxMalloc(void* user, size_t size)
{
	return asMalloc(size);
}

void ktxFree(void* user, void* memory)
{
	asFree(memory);
}

size_t ktxReadBlob(void* user, void* buffer, size_t byteCount)
{
	struct ktxFromBlobData* pData = (struct ktxFromBlobData*)user;
	size_t actualRead = byteCount <= pData->blobSize - pData->pos ? byteCount : pData->blobSize - pData->pos;
	memcpy(buffer, (unsigned char*)pData->pKtxBlob + pData->pos, actualRead);
	pData->pos += actualRead;
	return actualRead;
}

asColorFormat convertInternalFormat(TinyKtx_ContextHandle ctx)
{
	switch (TinyKtx_GetFormat(ctx)) {
	default: return AS_COLORFORMAT_MAX;
	case TIF_VK_FORMAT_R8G8B8A8_UNORM: return AS_COLORFORMAT_RGBA8_UNORM;
	case TIF_VK_FORMAT_R16G16B16A16_UNORM: return AS_COLORFORMAT_RGBA16_UNORM;
	case TIF_VK_FORMAT_R16G16B16A16_SFLOAT: return AS_COLORFORMAT_RGBA16_SFLOAT;
	case TIF_VK_FORMAT_R32G32B32A32_SFLOAT: return AS_COLORFORMAT_RGBA32_SFLOAT;
	case TIF_VK_FORMAT_A2R10G10B10_UNORM_PACK32: return AS_COLORFORMAT_A2R10G10B10_UNORM;
	case TIF_VK_FORMAT_B10G11R11_UFLOAT_PACK32: return AS_COLORFORMAT_B10G11R11_UFLOAT;
	case TIF_VK_FORMAT_R8_UNORM: return AS_COLORFORMAT_R8_UNORM;
	case TIF_VK_FORMAT_R16_UNORM: return AS_COLORFORMAT_R16_UNORM;
	case TIF_VK_FORMAT_R16_SFLOAT: return AS_COLORFORMAT_R16_SFLOAT;
	case TIF_VK_FORMAT_R32_SFLOAT: return AS_COLORFORMAT_R32_SFLOAT;
	case TIF_VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return AS_COLORFORMAT_BC1_RGBA_UNORM_BLOCK;
	case TIF_VK_FORMAT_BC3_UNORM_BLOCK: return AS_COLORFORMAT_BC3_UNORM_BLOCK;
	case TIF_VK_FORMAT_BC5_UNORM_BLOCK: return AS_COLORFORMAT_BC5_UNORM_BLOCK;
	case TIF_VK_FORMAT_BC6H_UFLOAT_BLOCK: return AS_COLORFORMAT_BC6H_UFLOAT_BLOCK;
	case TIF_VK_FORMAT_BC7_UNORM_BLOCK: return AS_COLORFORMAT_BC7_UNORM_BLOCK;
	}
}

#define KTX_TMP_RETURNIFERROR() if(blobData.error != AS_SUCCESS){return blobData.error;}
ASEXPORT asResults asTextureDesc_FromKtxData(asTextureDesc_t* pOut, void* pKtxBlob, size_t blobSize)
{
	/*Defaults*/
	*pOut = asTextureDesc_Init();

	/*Create Ktx Read Context*/
	struct ktxFromBlobData blobData = { 0, pKtxBlob, blobSize, AS_SUCCESS };
	TinyKtx_ContextHandle ctx = TinyKtx_CreateContext(&(TinyKtx_Callbacks) 
	{
		.errorFn = ktxError,
		.tellFn = ktxTellBlob,
		.seekFn = ktxSeekBlob,
		.readFn = ktxReadBlob,
		.allocFn = ktxMalloc,
		.freeFn = ktxFree
	},
	&blobData);
	KTX_TMP_RETURNIFERROR();
	
	/*Read Header*/
	TinyKtx_ReadHeader(ctx);
	KTX_TMP_RETURNIFERROR();

	/*Fill out Extents*/
	const uint32_t width = pOut->width = TinyKtx_Width(ctx);
	const uint32_t height = pOut->height = TinyKtx_Height(ctx);
	const uint32_t depth = TinyKtx_Depth(ctx) > 0 ? TinyKtx_Depth(ctx) : 1;
	const uint32_t slices = TinyKtx_ArraySlices(ctx) > 0 ? TinyKtx_ArraySlices(ctx) : 1;
	const uint32_t completeDepth = pOut->depth = (depth > 1 ? depth : slices);
	const uint32_t mipCount = TinyKtx_NumberOfMipmaps(ctx);
	pOut->mips = mipCount;

	/*Decide on Type*/
	if (TinyKtx_Is2D(ctx)) {
		if (TinyKtx_IsArray(ctx)) {
			if (TinyKtx_IsCubemap(ctx)) { pOut->type = AS_TEXTURETYPE_CUBEARRAY; }
			else { pOut->type = AS_TEXTURETYPE_2DARRAY; }
		}
		else {
			if (TinyKtx_IsCubemap(ctx)) { pOut->type = AS_TEXTURETYPE_CUBE; }
			else { pOut->type = AS_TEXTURETYPE_2D; }
		}
	}
	else if (TinyKtx_Is3D(ctx)) { pOut->type = AS_TEXTURETYPE_3D; }
	else { return AS_FAILURE_UNKNOWN_FORMAT; }

	/*Decide on Format*/
	pOut->format = convertInternalFormat(ctx);
	if (pOut->format == AS_COLORFORMAT_MAX) { return AS_FAILURE_UNKNOWN_FORMAT; }
	
	/*Get Dest Image Size*/
	size_t finalImageSize = 0;
	for (uint32_t i = 0; i < mipCount; i++){ finalImageSize += TinyKtx_ImageSize(ctx, i);}
	pOut->initialContentsBufferSize = finalImageSize;
	pOut->pInitialContentsBuffer = asMalloc(finalImageSize);
	pOut->initialContentsRegionCount = mipCount;

	size_t buffStart = 0;
	uint32_t w = width, h = height, d = depth;
	for (uint32_t i = 0; i < mipCount; i++)
	{
		const imgBuffSize = TinyKtx_ImageSize(ctx, i);
		asTextureContentRegion_t* region = &pOut->arrInitialContentsRegions[i];
		region->extent[0] = w;
		region->extent[1] = h;
		region->extent[2] = d;
		region->layer = 0;
		region->layerCount = slices;
		region->mipLevel = i;
		memset(region->offset, 0, sizeof(uint32_t) * 3);
		region->bufferStart = buffStart;
		const void* mipData = TinyKtx_ImageRawData(ctx, i);
		memcpy((unsigned char*)pOut->pInitialContentsBuffer + buffStart, mipData, imgBuffSize);

		if (w > 1) w = w / 2;
		if (h > 1) h = h / 2;
		if (d > 1) d = d / 2;
		buffStart += imgBuffSize;
	}

	KTX_TMP_RETURNIFERROR();
	return AS_SUCCESS;
}

ASEXPORT void asTextureDesc_FreeKtxData(asTextureDesc_t* pOut)
{
	asFree(pOut->pInitialContentsBuffer);
}
