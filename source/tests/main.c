#include "engine/engineCore/asEntry.h"
#include "engine/renderer/asRendererCore.h"
#include "engine/resource/asResource.h"
#include "engine/common/preferences/asPreferences.h"
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
//#include "engine/flecs/asFlecsImplimentation.h"
#include "engine/renderer/asTextureFromKtx.h"
#include "engine/renderer/asBindlessTexturePool.h"

#include "engine/renderer/asSceneRenderer.h"
#include "engine/model/runtime/asModelRuntime.h"

#include "engine/input/asInput.h"

#include "engine/gameUtilities/asFlyCamera.h"

asInputPlayer mainPlayerInput;

asTextureHandle_t texture;
asBufferHandle_t vBuffer;
uint32_t vtxCount;
asBufferHandle_t iBuffer;
uint32_t idxCount;
asShaderFx* pStandardSurfaceShader;
asResourceFileID_t standardSurfaceShaderFileID;

asPrimitiveSubmissionQueue subQueue;

float renderDebug = -1;
vec3 cameraPos = { 0, 0, -1 };
quat cameraRot = ASQUAT_IDENTITY;
vec2 cameraLook;
float fov = 90.0f;
void onUpdate(double deltaTime)
{
	if (igBegin("Hello Triangle", NULL, 0))
	{
		int debugValue = (int)renderDebug;
		if (igDragInt("RenderDebug", &debugValue, 0.1f, -1, 7, NULL))
		{
			renderDebug = (float)debugValue;
		}

		igDragFloat2("CameraLook", cameraLook, 0.1f, -FLT_MAX, FLT_MAX, NULL, 1.0f);
		igDragFloat3("Camera Position", cameraPos, 0.1f, -FLT_MAX, FLT_MAX, NULL, 1.0f);
		igDragFloat4("Camera Quaternion", cameraRot, 0.1f, -FLT_MAX, FLT_MAX, NULL, 1.0f);
		igDragFloat("FOV", &fov, 0.1f, 1.0f, 180.0f, NULL, 1.0f);
	}
	igEnd();

	/*Node Editor Test*/
	if (igBegin("Node Editor Test", NULL, 0))
	{
		inodeBeginNodeEditor();

		const int nodeId = 1;
		const int inputAttrId = 2;
		inodeBeginNode(nodeId);

		inodeBeginNodeTitleBar();
		igText("Hello Node!");
		inodeEndNodeTitleBar();

		inodeBeginOutputAttribute(inputAttrId, ImNodesPinShape_PinShape_Circle);
		igText("Output");
		inodeEndAttribute();

		inodeEndNode();

		inodeEndNodeEditor();
	}
	igEnd();
	/*Debug Drawing*/
	{
		for(int i = 0; i < 126; i++ )
		{
			float yPos = (1.0f / (float)126) * i;
			asDebugDrawLine3D(
			(vec3) { -1.0f, yPos, 1.0f },
			(vec3) { 1.0f, yPos, 1.0f },
			1.0f,
			(vec4) { 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}
	/*Input Test*/
	{
		if (asInputGetButtonDown(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "FlyCam"))) {
			asInputMouseSetRelative(true);
		}
		else if (asInputGetButtonUp(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "FlyCam"))) {
			asInputMouseSetRelative(false);
		}
		if (asInputGetButton(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "FlyCam")))
		{
			vec3 movement;
			movement[0] = asInputGetAxisClamped(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "MoveRight"));
			movement[1] = asInputGetAxisClamped(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "MoveUp"));
			movement[2] = asInputGetAxisClamped(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "MoveForward"));
			float lookX = asInputGetAxis(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "LookX"));
			float lookY = asInputGetAxis(mainPlayerInput, asInputGetMappingHandle(mainPlayerInput, "LookY"));

			float sensitivity = 360.0f;

			asGameUtils_FlyCamera(deltaTime, movement, lookX, lookY, &cameraLook[0], &cameraLook[1], sensitivity, 2.5f, cameraPos, cameraRot);
		}
	}

	/*Mid-frame Texture Submission Test*/
	static float texUploadTimer = 0;
	if (texUploadTimer > 5.0f) /*Wait 5 Seconds*/
	{
		asTexturePoolAddFromHandle(texture, NULL);
		texUploadTimer = -1.0;
	} else if (texUploadTimer >= 0.0f) { texUploadTimer += deltaTime; }

	/*Set Viewport*/
	asGfxViewerParamsDesc viewParams = { 0 };
	viewParams.fov = fov;
	viewParams.clipStart = 0.001f;
	viewParams.clipEnd = 10000.0f;
	viewParams.debugState = (int32_t)renderDebug;
	glm_vec3_copy(cameraPos, viewParams.viewPosition);
	glm_quat_copy(cameraRot, viewParams.viewRotation);
	asSceneRendererSetViewerParams(1, &viewParams);

	/*Begin Recording*/
	asSceneRendererSubmissionQueuePopulateBegin(subQueue);

	/*Transforms*/
	uint32_t transformOffset;
	asGfxInstanceTransform transform = {
		.position = {0.0f, 0.0f, 0.0f},
		.rotation = {0.0f, 0.0f, 0.0f, 1.0f},
		.scale = {1.0f, 1.0f, 1.0f},
		.opacity = 1.0f
	};
	asSceneRendererSubmissionAddTransforms(subQueue, 1, &transform, &transformOffset);

	/*Primitive*/
	asGfxPrimativeGroupDesc primDesc = { 0 };
	memset(&primDesc, 0, sizeof(primDesc));
	primDesc.vertexBuffer = vBuffer;
	primDesc.vertexCount = vtxCount;
	primDesc.indexBuffer = iBuffer;
	primDesc.indexCount = idxCount;
	primDesc.sortDistance = 0.0f;
	primDesc.renderPass = AS_SCENE_RENDERPASS_SOLID;
	primDesc.baseInstanceCount = 1;
	primDesc.materialId = 0;
	primDesc.debugState = 0;
	primDesc.transformCount = 1;
	primDesc.transformOffset = transformOffset;
	primDesc.transformOffsetPreviousFrame = transformOffset;
	primDesc.pShaderFx = pStandardSurfaceShader;
	asSceneRendererSubmissionAddPrimitiveGroups(subQueue, 1, &primDesc);

	/*End Queue*/
	asSceneRendererSubmissionQueuePopulateEnd(subQueue);

	if (renderDebug > 0) { asSceneRendererSubmissionQueuePrepareFrameSubmit(subQueue); }
}

void onExit(void)
{
	asReleaseBuffer(vBuffer);
	asReleaseBuffer(iBuffer);
	asReleaseTexture(texture);
	asShaderFxManagerDereferenceShaderFx(standardSurfaceShaderFileID);
	asSceneRendererDestroySubmissionQueue(subQueue);
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
	return AS_SUCCESS;
}

asResults doReflectTest(const char* propName, void* pCurrentValue, void* pNewValueTmp, void* pUserData)
{
	reflectTest();
	return AS_SUCCESS;
}

typedef struct TestComponent2
{
	float doot;
} TestComponent2;

//void ecsTest(ecs_rows_t* rows) {
//	ECS_COLUMN(rows, TestComponent2, dat, 1);
//
//	for (int i = 0; i < rows->count; i++)
//	{
//		//asDebugLog("Doot #%d = %f", i, dat[i].doot);
//	}
//}

//asResults addEntityTest(const char* propName, void* pCurrentValue, void* pNewValueTmp, void* pUserData)
//{
//	ecs_world_t* world = asEcsGetWorldPtr();
//	return AS_SUCCESS;
//}

int main(int argc, char* argv[])
{
	asAppInfo_t appInfo = (asAppInfo_t){ 0 };
	appInfo.pAppName = "astrengine_Testbed";
	appInfo.pDevName = "Alex Strand";
	appInfo.appVersion.major = 1; appInfo.appVersion.minor = 0; appInfo.appVersion.patch = 0;
	atexit(onExit);
	asIgnite(argc, argv, &appInfo, NULL);

	/*Test Preference System*/
	{
		asPreferencesRegisterOpenSection(asGetGlobalPrefs(), "test");
		asPreferencesRegisterParamFloat(asGetGlobalPrefs(), "testFloat", &testFloat, 0.0f, 1000.0f, false, testCb, NULL, NULL);
		asPreferencesRegisterParamFloat(asGetGlobalPrefs(), "drawDebug", &renderDebug, -FLT_MAX, FLT_MAX, true, NULL, NULL, NULL);
		asPreferencesRegisterParamCString(asGetGlobalPrefs(), "testString", testStr, 80, false, NULL, NULL, NULL);
		asPreferencesRegisterNullFunction(asGetGlobalPrefs(), "showAsciiArt", showAscii, NULL, NULL);
		asPreferencesRegisterNullFunction(asGetGlobalPrefs(), "reflectTest", doReflectTest, NULL, NULL);
		//asPreferencesRegisterParamFloat(asGetGlobalPrefs(), "addEntityTest", NULL, -FLT_MAX, FLT_MAX, false, addEntityTest, NULL, NULL);
		asPreferencesLoadSection(asGetGlobalPrefs(), "test");

		asDebugLog("Test String:%.*s", 80, testStr);
	}
	/*Setup Player*/
	{
		asInputMappingDesc inputs[] = {
			{
				.name = "FlyCam",
				.mouseButtonPositive = "Right"
			},
			{
				.name = "MoveRight",
				.keyPostivie = "D",
				.keyNegative = "A"
			},
			{
				.name = "MoveForward",
				.keyPostivie = "W",
				.keyNegative = "S"
			},
			{
				.name = "MoveUp",
				.keyPostivie = "Q",
				.keyNegative = "E"
			},
			{
				.name = "LookX",
				.mouseAxis = "X"
			},
			{
				.name = "LookY",
				.mouseAxis = "Y"
			},
		};

		asInputPlayerDesc playerDesc;
		playerDesc.controllerIndex = 0;
		playerDesc.playerConfigFileName = "playerInput_0.ini";
		playerDesc.ownsKeyboardMouse = true;
		playerDesc.inputMappingCount = ASARRAYLEN(inputs);
		playerDesc.pInputMappings = inputs;
		asInputCreatePlayer(&mainPlayerInput, &playerDesc);
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
		asResourceType_t resourceType_Texture = asResource_RegisterType("TEXTURE", 7);
		asResourceLoader_t file;
		asResourceFileID_t resID;
		if (asResourceLoader_OpenByPath(&file, &resID, "test_image", 11) != AS_SUCCESS)
		{
			asFatalError("Failed to open image");
		}

		size_t fSize = asResourceLoader_GetContentSize(&file);
		unsigned char* fContents = asMalloc(fSize);
		asResourceLoader_ReadAll(&file, fSize, fContents);
		asResourceLoader_Close(&file);

		/*Todo: Load KTX*/
		asTextureDesc_t desc = asTextureDesc_Init();
		asTextureDesc_FromKtxData(&desc, fContents, fSize);
		texture = asCreateTexture(&desc);
		asTextureDesc_FreeKtxData(&desc);

		//asTexturePoolAddFromHandle(texture, NULL);

		asResourceDataMapping_t map = { .hndl = texture };
		asResource_Create(resID, map, resourceType_Texture, 3);
		asResource_DeincrimentReferences(resID, 1);
		asResource_DeincrimentReferences(resID, 1);
		asResource_DeincrimentReferences(resID, 3);

		asResourceDataMapping_t* deleteQueue;
		size_t count = asResource_GetDeletionQueue(resourceType_Texture, NULL, &deleteQueue);
		for (size_t i = 0; i < count; i++)
		{
			//asReleaseTexture(deleteQueue[i].hndl);
		}
	}
	/*Test buffer creation*/
	{
		/*asBufferDesc_t desc = asBufferDesc_Init();
		desc.bufferSize = 1024;
		desc.cpuAccess = AS_GPURESOURCEACCESS_STREAM;
		desc.usageFlags = AS_BUFFERUSAGE_VERTEX;
		desc.initialContentsBufferSize = desc.bufferSize;
		desc.pInitialContentsBuffer = asMalloc(desc.initialContentsBufferSize);
		memset(desc.pInitialContentsBuffer, 0, desc.initialContentsBufferSize);
		desc.pDebugLabel = "TestBuffer";
		buffer = asCreateBuffer(&desc);*/
		///*Hash test*/
		//asHash64_t hash = asHashBytes64_xxHash(desc.pInitialContentsBuffer, desc.initialContentsBufferSize);
		//asFree(desc.pInitialContentsBuffer);
	}
	/*Setup ECS for Testing*/
	/*{
		ecs_world_t* world = asEcsGetWorldPtr();
		ECS_COMPONENT(world, TestComponent2);
		ECS_SYSTEM(world, ecsTest, EcsOnUpdate, TestComponent2);

		ECS_ENTITY(world, MyEntity, TestComponent2);
		ecs_set(world, MyEntity, TestComponent2, { 65 });
	}*/
	/*Super Hello Triange*/
	{
#define triCount 64
		/*Triangles*/
		asVertexGeneric vtx[3 * triCount] = { 0 };
		uint16_t indices[3 * triCount];
		vtxCount = ASARRAYLEN(vtx);
		idxCount = ASARRAYLEN(indices);
		uint8_t vColors[3][4] = {
			{ 255, 0, 0, 255 },
			{ 0, 255, 0, 255 },
			{ 0, 0, 255, 255 },
		};
		float vPos[3][3] = {
			{0.0f, 0.5f, 0.0f},
			{0.5f, -0.5f, 0.0f},
			{-0.5f, -0.5f, 0.0f},
		};
		vec3 vNormal[3] = {
			{-1.0f, -1.0f, 1.0f},
			{-1.0f, -1.0f, 1.0f},
			{-1.0f, -1.0f, 1.0f},
		};
		float vTan[3][4] = {
			{0.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 0.0f, 1.0f},
		};
		float vUV[3][2] = {
			{0.5f, 0.0f},
			{1.0f, 1.0f},
			{0.0f, 1.0f},
		};

		float zOffset = 0.0f;
		for (int tri = 0; tri < triCount; tri++)
		{
			for (int i = 0; i < 3; i++)
			{
				float posCalc[3] = { vPos[i][0], vPos[i][1], vPos[i][2] + zOffset };
				asVertexGeneric_encodePosition(&vtx[(tri * 3) + i], posCalc);
				asVertexGeneric_encodeColor(&vtx[(tri * 3) + i], vColors[i]);
				asVertexGeneric_encodeNormal(&vtx[(tri * 3) + i], vNormal[i]);
				asVertexGeneric_encodeTangent(&vtx[(tri * 3) + i], vTan[i], (int)vTan[i][3]);
				asVertexGeneric_encodeUV(&vtx[(tri * 3) + i], 0, vUV[i]);
				indices[(tri * 3) + i] = (tri * 3) + i;
			}
			zOffset -= 1.0f;
		}

		/*Buffers*/
		asBufferDesc_t vBuffDesc = asBufferDesc_Init();
		vBuffDesc.bufferSize = sizeof(vtx);
		vBuffDesc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		vBuffDesc.usageFlags = AS_BUFFERUSAGE_VERTEX;
		vBuffDesc.initialContentsBufferSize = vBuffDesc.bufferSize;
		vBuffDesc.pInitialContentsBuffer = vtx;
		vBuffDesc.pDebugLabel = "VertexBuffer";
		vBuffer = asCreateBuffer(&vBuffDesc);

		asBufferDesc_t iBuffDesc = asBufferDesc_Init();
		iBuffDesc.bufferSize = sizeof(indices);
		iBuffDesc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		iBuffDesc.usageFlags = AS_BUFFERUSAGE_INDEX;
		iBuffDesc.initialContentsBufferSize = iBuffDesc.bufferSize;
		iBuffDesc.pInitialContentsBuffer = indices;
		iBuffDesc.pDebugLabel = "IndexBuffer";
		iBuffer = asCreateBuffer(&iBuffDesc);

		/*Shader*/
		const char* path = "shaders/core/StandardScene_FX.asfx";
		standardSurfaceShaderFileID = asResource_FileIDFromRelativePath(path, strlen(path));
		pStandardSurfaceShader = asShaderFxManagerGetShaderFx(standardSurfaceShaderFileID);

		/*Submission Queue*/
		asPrimitiveSubmissionQueueDesc desc = { 0 };
		desc.maxTransforms = 1;
		desc.maxInstances = 1;
		desc.maxPrimitives = 1;
		subQueue = asSceneRendererCreateSubmissionQueue(&desc);
	}

	asLoopDesc_t loopDesc;
	loopDesc.fpOnUpdate = (asUpdateFunction_t)onUpdate;
	loopDesc.fpOnTick = (asUpdateFunction_t)NULL;

	return asEnterLoop(loopDesc);
}