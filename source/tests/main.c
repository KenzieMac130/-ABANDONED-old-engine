#include "engine/asEntry.h"
#include "engine/asRendererCore.h"

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
		desc.format = AS_TEXTUREFORMAT_R8_UNORM;
		desc.depth = 1;
		desc.mips = 1;
		desc.maxLod = FLT_MAX;
		desc.maxAnisotropy = 16;
		desc.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
		desc.cpuAccess = AS_GPURESOURCEACCESS_IMMUTABLE;
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
		texture = asCreateTexture(&desc);
		asFree(desc.pInitialContentsBuffer);
		asReleaseTexture(texture);
	}

	return asEnterLoop();
}