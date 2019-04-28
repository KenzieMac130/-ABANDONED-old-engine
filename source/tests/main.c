#include "engine/asEntry.h"
#include "engine/asRendererCore.h"

#include "engine/asNuklearImplimentation.h"
#define NK_IMPLEMENTATION
#include "../thirdparty/nuklear/nuklear.h"

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
			//asHandle_t hndl = asCreateHandle(&man);
			if (i >= 50)
			{
				//asDestroyHandle(&man, hndl);
			}
		}
		asHandleManagerDestroy(&man);
	}
	/*Test texture creation*/
	{
		asTextureHandle_t texture;
		asTextureDesc_t desc = asTextureDesc_Init();
		desc.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
		desc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		desc.format = AS_COLORFORMAT_RGBA8_UNORM;
		desc.width = 512;
		desc.height = 512;
		desc.initialContentsBufferSize = 512 * 512 * 4;
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
		desc.pInitialContentsBuffer = asMalloc(desc.initialContentsBufferSize);
		memset(desc.pInitialContentsBuffer, 0, desc.initialContentsBufferSize);
		desc.pDebugLabel = "TestImage";
		texture = asCreateTexture(&desc);
		asFree(desc.pInitialContentsBuffer);
		asReleaseTexture(texture);
	}
	/*Test buffer creation*/
	{
		asBufferHandle_t buffer;
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
		asReleaseBuffer(buffer);
	}
	/*Test shader creation*/
	{
		//asShaderFxDesc_t shaderDesc = asShaderDesc_Init("Test", 5);
	}
	/*Test nuklear, this should be in a loop tho...*/
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
	return asEnterLoop();
}