#include "reflectTest.h"

#include "engine/common/reflection/asReflectImpliment.h"
asReflectContainer GameObjectReflectData = REFLECT_MACRO_GameObject_test;

#include "engine/common/reflection/asReflectIOBinary.h"
#include "engine/common/reflection/asManualSerialList.h"

#include "engine/common/asBin.h"

/*Extra Gameplay Specific Data (we chose not to expose this via reflection)*/
typedef struct {
	int32_t lives;
	int32_t score;
	bool isPoweredUp;
	bool isDying;
	int32_t currentAnimationFrame;
} PacManState;
PacManState mainPlayerState;

/*Setup Main Player*/
GameObject_test PlayerOne = {
	.name = "PlayerOne",

	.position = {0.0f,500.0f,0.0f},
	.rotationEuler = {0.0f, 90.0f, 0.0f},
	.scale = {1.0f, 1.0f, 1.0f},
	.velocity = {0.4f, 0.2f, 5.0f},

	.objTypeIdx = GAMEOBJECT_TYPE_PACMAN,
	.pGameplayData = &mainPlayerState,
};

#define GHOST_COUNT 4
GameObject_test Ghosts[GHOST_COUNT] = {
	{
	.name = "Inky",

	.position = {0.0f,500.0f,0.0f},
	.rotationEuler = {0.0f, 90.0f, 0.0f},
	.scale = {1.0f, 1.0f, 1.0f},
	.velocity = {0.4f, 0.2f, 5.0f},

	.objTypeIdx = GAMEOBJECT_TYPE_INKY,
	.pGameplayData = NULL,
	},
	{
	.name = "Blinky",

	.position = {0.0f,500.0f,0.0f},
	.rotationEuler = {0.0f, 90.0f, 0.0f},
	.scale = {1.0f, 1.0f, 1.0f},
	.velocity = {0.4f, 0.2f, 5.0f},

	.objTypeIdx = GAMEOBJECT_TYPE_BLINKY,
	.pGameplayData = NULL,
	},
	{
	.name = "Pinky",

	.position = {0.0f,500.0f,0.0f},
	.rotationEuler = {0.0f, 90.0f, 0.0f},
	.scale = {1.0f, 1.0f, 1.0f},
	.velocity = {0.4f, 0.2f, 5.0f},

	.objTypeIdx = GAMEOBJECT_TYPE_PINKY,
	.pGameplayData = NULL,
	},
	{
	.name = "Clyde",

	.position = {0.0f,500.0f,0.0f},
	.rotationEuler = {0.0f, 90.0f, 0.0f},
	.scale = {1.0f, 1.0f, 1.0f},
	.velocity = {0.4f, 0.2f, 5.0f},

	.objTypeIdx = GAMEOBJECT_TYPE_CLYDE,
	.pGameplayData = NULL,
	}
};

asHandleManager_t renderMeshHandleManager;
void reflectTest()
{
	/*Startup theoretical Model Manager*/
	asHandleManagerCreate(&renderMeshHandleManager, RENDER_MESH_MAX);

	/*No Parent/Child Relationship... PACMAN IS AN ORPHAN!!!*/
	PlayerOne.firstChildHndl = asHandle_Invalidate();
	PlayerOne.nextSiblingHndl = asHandle_Invalidate();
	PlayerOne.parentHndl = asHandle_Invalidate();

	/*Imagine this is a loading a 3D model from the hard drive*/
	PlayerOne.renderMeshFileID = asResource_FileIDFromRelativePath("PacmanModel.asmdl", 17);
	asCreateHandle(&renderMeshHandleManager);
	asCreateHandle(&renderMeshHandleManager);
	PlayerOne.renderMeshHandle = asCreateHandle(&renderMeshHandleManager);
	/*...more resource stuff here...*/

	/*Cleanup The Model Manager... Wow... Really Short Game!*/
	asHandleManagerDestroy(&renderMeshHandleManager);

	/*Lets read the results of the structure*/
	asDebugLog("-----------REFLECTION TEST-----------");
	asDebugLog("Structure Name: %s (size:%u)", GameObjectReflectData.name, GameObjectReflectData.binarySize);
	int structMemberCount = asReflectContainerMemberCount(&GameObjectReflectData);
	for (int i = 0; i < structMemberCount; i++)
	{
		asDebugLog("Member: %s %s (offset:%u size:%u)", 
			GameObjectReflectData.data[i].typeName,
			GameObjectReflectData.data[i].varName,
			GameObjectReflectData.data[i].dataOffset,
			GameObjectReflectData.data[i].dataSize);
	}

	/*File Saving*/
	asBinWriter writer;
	asBinWriterOpen(&writer, "ECS", "testFile.asbin", 4);

	/*Dump the Structure*/
	size_t binSize = asReflectGetBinarySize(&GameObjectReflectData, GHOST_COUNT);
	unsigned char* binDump = asMalloc(binSize);
	asReflectSaveToBinary(binDump, binSize, &GameObjectReflectData, Ghosts, GHOST_COUNT);
	asBinWriterAddSection(&writer, (asBinSectionIdentifier) { "GHOSTS", 0 }, binDump, binSize);
	asFree(binDump);

	/*Save asbin*/
	asBinWriterClose(&writer);
	/*Load asbin*/
	FILE* fp = fopen("testFile.asbin", "rb");
	ASASSERT(fp);
	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	void* fileContent = asMalloc(fileSize);
	ASASSERT(fileContent);
	fread(fileContent, fileSize, 1, fp);
	fclose(fp);

	/*Fetch Serialization Data from asbin*/
	asBinReader reader;
	binDump = NULL;
	asBinReaderOpenMemory(&reader, "ECS", fileContent, fileSize);
	asBinReaderGetSection(&reader, (asBinSectionIdentifier) { "GHOSTS", 0 }, &binDump, &binSize);

	/*Load Test*/
	GameObject_test loadObjects[7];
	memset(loadObjects, 0, sizeof(loadObjects));
	strcat(loadObjects[6].name, "Boo");
	uint32_t count;
	asReflectLoadFromBinary(loadObjects, sizeof(GameObject_test), 7, &GameObjectReflectData, binDump, binSize, &count, NULL);

	asFree(fileContent);

	/*Manual content saving*/
	asManualSerialList manualList;
	asManualSerialListCreate(&manualList, "GameObject_test", 2, sizeof(GameObject_test), 1);
	asManualSerialListAddProperty(&manualList, "char[]", "name", "NewName", 8);

	binSize = asReflectGetBinarySize(&manualList.reflectContainer, 4);
	binDump = asMalloc(binSize);
	asReflectSaveToBinary(binDump, binSize, &manualList.reflectContainer, manualList.pData, 1);
	asManualSerialListFree(&manualList);

	asReflectLoadFromBinary(loadObjects+4, sizeof(GameObject_test), 1, &GameObjectReflectData, binDump, binSize, NULL, NULL);
	asFree(binDump);

	/*Print Ghosts*/
	for (int i = 0; i < 7; i++)
	{
		asDebugLog("Ghost: %s", loadObjects[i].name);
	}
}