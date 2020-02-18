#include "reflectTest.h"

#include "engine/common/reflection/asReflectImpliment.h"
asReflectContainer GameObjectReflectData = REFLECT_MACRO_GameObject_test;

#include "engine/common/reflection/asReflectIOBinary.h"

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

GameObject_test Ghosts[] = { 
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

	/*Dump the Structure*/
	size_t binSize = asReflectGetBinarySize(&GameObjectReflectData, 4);
	unsigned char* binDump = asMalloc(binSize);
	asReflectSaveToBinary(binDump, binSize, &GameObjectReflectData, Ghosts, 4);

	/*Load Test*/
	GameObject_test loadObjects[7];
	memset(loadObjects, 0, sizeof(loadObjects));
	strcat(loadObjects[6].name, "Boo");
	uint32_t count;
	asReflectLoadFromBinary(loadObjects, sizeof(GameObject_test), 7, &GameObjectReflectData, binDump, binSize, &count, NULL);
	for (int i = 0; i < 7; i++)
	{
		asDebugLog("Ghost: %s", loadObjects[i].name);
	}
	asFree(binDump);
}