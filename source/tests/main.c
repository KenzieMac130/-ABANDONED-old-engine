#include "engine/engineCore/asEntry.h"
#include "engine/renderer/asRendererCore.h"
#include "engine/resource/asResource.h"
#include "engine/preferences/asPreferences.h"
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

#include "engine/guiTools/cmdConsole/asCmdConsole.h"

void onUpdate(double time)
{
	
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

/*Pref Test*/
static float testFloat;
static char testStr[80];
asResults testCb(const char* propName, void* pCurrentValue, void* pNewValueTmp, void* pUserData)
{
	float* pNewFloat = (float*)pNewValueTmp;
	float* pCurFloat = (float*)pCurrentValue;
	asDebugLog("test Float is: %f, was: %f", *pNewFloat, *pCurFloat);
	*pCurFloat = *pNewFloat;
	return AS_SUCCESS;
}

asResults showAscii(const char* propName, void* pCurrentValue, void* pNewValueTmp, void* pUserData)
{
	asDebugLog("               __ .-~-.   .~``~,._\n"
		"             .~  `     \\ /     .  `\\\n"
		"             |     \\    |   .'     |\n"
		"       _      \\     `.  |  /    __/\n"
		"    .~` `'. .--;.       ,.O  -~`   `\\\n"
		"    \\  '-. |     `-  o.O/o, __       |\n"
		"     '-.,__ \\    .-~' `\\|o `  ~.    /_\n"
		"       _.--'/   `    ,  /  \\        | `~-.,\n"
		"      /     |       /  :    '._    / -.    `\\\n"
		"jgs  /   .-' '.___.;   `      \\`--'\\    `    |\n"
		"    |          /    \\         /     '.__,.--,/\n"
		"    '--..,___.'      `~--'--~'");
}

int main(int argc, char* argv[])
{
	asAppInfo_t appInfo = (asAppInfo_t){0};
	appInfo.pAppName = "astrengine_Testbed";
	appInfo.pDevName = "Alex Strand";
	appInfo.appVersion.major = 1; appInfo.appVersion.minor = 0; appInfo.appVersion.patch = 0;
	atexit(asShutdown);
	asIgnite(argc, argv, &appInfo, NULL);
	/*Test Preference System*/
	{
		asPreferencesRegisterOpenSection(asGetGlobalPrefs(), "test");
		asPreferencesRegisterParamFloat(asGetGlobalPrefs(), "testFloat", &testFloat, 0.0f, 1000.0f, false, testCb, NULL, NULL);
		asPreferencesRegisterParamCString(asGetGlobalPrefs(), "testString", testStr, 80, false, NULL, NULL, NULL);
		asPreferencesRegisterNullFunction(asGetGlobalPrefs(), "showAsciiArt", showAscii, false, NULL, NULL);
		strncat(testStr, "I am string", 79);
		asPreferencesLoadSection(asGetGlobalPrefs(), "test");

		asDebugLog("%.*s", 80, testStr);
	}
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

	reflectTest();

	asLoopDesc_t loopDesc;
	loopDesc.fpOnUpdate = (asUpdateFunction_t)onUpdate;
	loopDesc.fpOnTick = (asUpdateFunction_t)NULL;
	return asEnterLoop(loopDesc);
}