#include "engine/asEntry.h"
#include "engine/asRendererCore.h"
#include "engine/asResource.h"

#include "engine/asNuklearImplimentation.h"
#define NK_IMPLEMENTATION
#include "../thirdparty/nuklear/nuklear.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb/stb_image.h"

void onUpdate(double time)
{
	/*Test nuklear*/
	{
		struct nk_context *pNkCtx = asGetNuklearContextPtr();
		enum { EASY, HARD };
		static int op = EASY;
		static float value = 0.6f;
		static int i = 20;

		if (nk_begin(pNkCtx, "Show", nk_rect(50, 50, 220, 220),
			NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE)) {
			/* fixed widget pixel width */
			nk_layout_row_static(pNkCtx, 30, 80, 1);
			if (nk_button_label(pNkCtx, "button")) {
				/* event handling */
			}

			/* fixed widget window ratio width */
			nk_layout_row_dynamic(pNkCtx, 30, 2);
			if (nk_option_label(pNkCtx, "easy", op == EASY)) op = EASY;
			if (nk_option_label(pNkCtx, "hard", op == HARD)) op = HARD;

			/* custom widget pixel width */
			nk_layout_row_begin(pNkCtx, NK_STATIC, 30, 2);
			{
				nk_layout_row_push(pNkCtx, 50);
				nk_label(pNkCtx, "Volume:", NK_TEXT_LEFT);
				nk_layout_row_push(pNkCtx, 110);
				nk_slider_float(pNkCtx, 0, &value, 1.0f, 0.1f);
			}
			nk_layout_row_end(pNkCtx);
		}
		nk_end(pNkCtx);
	}
}

asTextureHandle_t texture;
asBufferHandle_t buffer;

void onExit()
{
	asReleaseTexture(texture);
	asReleaseBuffer(buffer);
	asShutdown();
}

int main(int argc, char* argv[])
{
	asAppInfo_t appInfo = (asAppInfo_t){0};
	appInfo.pAppName = "astrengine_Testbed";
	appInfo.appVersion.major = 1; appInfo.appVersion.minor = 0; appInfo.appVersion.patch = 0;
	atexit(asShutdown);
	asIgnite(argc, argv, &appInfo, NULL);

	/*Test handle manager*/
	{
		asHandleManager_t man;
		asHandleManagerCreate(&man, 8000);
		for (int i = 0; i < 8000; i++)
		{
			asHandle_t hndl = asCreateHandle(&man);
			if (i % 6 != 0)
			{
				asDestroyHandle(&man, hndl);
			}
		}
		asHandleManagerDestroy(&man);
	}
	/*Test texture creation*/
	{
		asResourceType_t resourceType_Texture = asResource_RegisterType("TEXTURE", 7);
		asResourceLoader_t file;
		asResourceFileID_t resID = asResource_FileIDFromRelativePath("Derp.jpg", 9);
		asResourceLoader_Open(&file, resID);
		size_t fSize = asResourceLoader_GetContentSize(&file);
		unsigned char* fContents = asMalloc(fSize);
		asResourceLoader_ReadAll(&file, fSize, fContents);
		asResourceLoader_Close(&file);

		int x, y, channels;
		unsigned char* img = stbi_load_from_memory(fContents, (int)fSize, &x, &y, &channels, 4);
		asFree(fContents);
		if (!img)
			asFatalError("Failed to load image");
		asTextureDesc_t desc = asTextureDesc_Init();
		desc.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
		desc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		desc.format = AS_COLORFORMAT_RGBA8_UNORM;
		desc.width = x;
		desc.height = y;
		desc.initialContentsBufferSize = x * y * 4;
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
		desc.pInitialContentsBuffer = img;
		desc.pDebugLabel = "TestImage";
		texture = asCreateTexture(&desc);
		stbi_image_free(img);

		asResourceDataMapping_t map = { .hndl = texture };
		asResource_Create(resID, map, resourceType_Texture, 3);
		asResource_DeincrimentReferences(resID, 1);
		asResource_DeincrimentReferences(resID, 1);
		asResource_DeincrimentReferences(resID, 3);

		asResourceDataMapping_t* deleteQueue;
		size_t count = asResource_GetDeletionQueue(resourceType_Texture, &deleteQueue);
		for (size_t i = 0; i < count; i++)
		{
			asReleaseTexture(deleteQueue[i].hndl);
		}
	}
	/*Test buffer creation*/
	{
		asBufferDesc_t desc = asBufferDesc_Init();
		desc.bufferSize = 1024;
		desc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
		desc.usageFlags = AS_BUFFERUSAGE_VERTEX;
		desc.initialContentsBufferSize = desc.bufferSize;
		desc.pInitialContentsBuffer = asMalloc(desc.initialContentsBufferSize);
		memset(desc.pInitialContentsBuffer, 0, desc.initialContentsBufferSize);
		desc.pDebugLabel = "TestBuffer";
		buffer = asCreateBuffer(&desc);
		/*Hash test*/
		asHash64_t hash = asHashBytes64_xxHash(desc.pInitialContentsBuffer, desc.initialContentsBufferSize);
		asFree(desc.pInitialContentsBuffer);
	}
	/*Test shader loading*/
	{
		asShaderFxHandle_t fx = asShaderFx_FromResource(asResource_FileIDFromRelativePath("ShaderFileMockup.asfx", 22));
		
	}
	asLoopDesc_t loopDesc;
	loopDesc.fpOnUpdate = (asUpdateFunction_t)onUpdate;
	loopDesc.fpOnTick = (asUpdateFunction_t)NULL;
	return asEnterLoop(loopDesc);
}