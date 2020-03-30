#include "engine/engineCore/asEntry.h"
#include "engine/renderer/asRendererCore.h"
#include "engine/resource/asResource.h"
#include "engine/common/asHandleManager.h"

#include "engine/nuklear/asNuklearImplimentation.h"
#if ASTRENGINE_NUKLEAR
#define NK_IMPLEMENTATION
#include "../thirdparty/nuklear/nuklear.h"
#endif
#if ASTRENGINE_DEARIMGUI
#include "engine/cimgui/asDearImGuiImplimentation.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb/stb_image.h"

#include "reflectTest.h"
#include "nuklearOverview.h"

void onUpdate(double time)
{
	/*Test Imgui*/
	static bool imguiDemoOpen;
	static bool windowOpen;
	static bool thing;
	static float value = 0.0f;
	static float color[4];
	igShowDemoWindow(imguiDemoOpen);

	igBegin("Astrengine Test Window", &windowOpen, 0);
	igSliderFloat("Test Float", &value, 0, 5.0f, NULL, 1);
	igTextColored((ImVec4) { 1.0f, 0.0f, 1.0f, 0.4f }, "Delta Time: %f", time);
	igShowUserGuide();
	igColorPicker4("Color Picker", color, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel, NULL);
	igEnd();
}

asTextureHandle_t texture;
asBufferHandle_t buffer;

void onExit()
{
	asReleaseTexture(texture);
	asReleaseBuffer(buffer);
	asShutdown();
}

/*Designated Initializer with Array Test*/
typedef struct {
	const uint32_t* data;
} dInitList;
dInitList dInK = { .data = (uint32_t []){0,9,2,} };

int main(int argc, char* argv[])
{
	asAppInfo_t appInfo = (asAppInfo_t){0};
	appInfo.pAppName = "astrengine_Testbed";
	appInfo.pDevName = "Alex Strand";
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
		//asResourceType_t resourceType_Texture = asResource_RegisterType("TEXTURE", 7);
		//asResourceLoader_t file;
		//asResourceFileID_t resID;
		//if (asResourceLoader_OpenByPath(&file, &resID, "TEST_IMAGE", 11) != AS_SUCCESS)
		//{
		//	asFatalError("Failed to open image");
		//}
		//
		//size_t fSize = asResourceLoader_GetContentSize(&file);
		//unsigned char* fContents = asMalloc(fSize);
		//asResourceLoader_ReadAll(&file, fSize, fContents);
		//asResourceLoader_Close(&file);

		///*Todo: Load KTX*/
		//void* img;

		//asFree(fContents);
		//if (!img)
		//{
		//	asFatalError("Failed to load image");
		//}
		//asTextureDesc_t desc = asTextureDesc_Init();
		//desc.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
		//desc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		//desc.format = AS_COLORFORMAT_RGBA8_UNORM;
		//desc.width = x;
		//desc.height = y;
		//desc.initialContentsBufferSize = (size_t)(x * y * 4);
		//desc.initialContentsRegionCount = 1;
		//asTextureContentRegion_t region = (asTextureContentRegion_t) { 0 };
		//region.bufferStart = 0;
		//region.extent[0] = desc.width;
		//region.extent[1] = desc.height;
		//region.extent[2] = 1;
		//region.layerCount = 1;
		//region.layer = 0;
		//region.mipLevel = 0;
		//desc.pInitialContentsRegions = &region;
		//desc.pInitialContentsBuffer = img;
		//desc.pDebugLabel = "TestImage";
		//texture = asCreateTexture(&desc);
		//stbi_image_free(img);

		//asResourceDataMapping_t map = { .hndl = texture };
		//asResource_Create(resID, map, resourceType_Texture, 3);
		//asResource_DeincrimentReferences(resID, 1);
		//asResource_DeincrimentReferences(resID, 1);
		//asResource_DeincrimentReferences(resID, 3);

		//asResourceDataMapping_t* deleteQueue;
		//size_t count = asResource_GetDeletionQueue(resourceType_Texture, &deleteQueue);
		//for (size_t i = 0; i < count; i++)
		//{
		//	asReleaseTexture(deleteQueue[i].hndl);
		//}
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
		/*asShaderFxHandle_t fx = asShaderFx_FromResource(asResource_FileIDFromRelativePath("ShaderFileMockup.asfx", 22));*/
		
	}

	reflectTest();

	asLoopDesc_t loopDesc;
	loopDesc.fpOnUpdate = (asUpdateFunction_t)onUpdate;
	loopDesc.fpOnTick = (asUpdateFunction_t)NULL;
	return asEnterLoop(loopDesc);
}