#pragma once

#include "engine/common/reflection/asReflectDefine.h"
#include "engine/common/asHandleManager.h"
#include "engine/resource/asResource.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define GAMEOBJECT_NAME_MAX 64
#define RENDER_MESH_MAX 1000

typedef enum {
	GAMEOBJECT_TYPE_STATIC_MODEL = 0,
	GAMEOBJECT_TYPE_PELLET = 1,
	GAMEOBJECT_TYPE_POWER_PELLET = 2,
	GAMEOBJECT_TYPE_PACMAN = 3,
	GAMEOBJECT_TYPE_INKY = 4,
	GAMEOBJECT_TYPE_BLINKY = 5,
	GAMEOBJECT_TYPE_PINKY = 5,
	GAMEOBJECT_TYPE_CLYDE = 6,
} GameObjectClassesIndex_t;

#define REFLECT_MACRO_GameObject_test AS_REFLECT_STRUCT(GameObject_test, \
	/*Name*/\
	AS_REFLECT_ENTRY_ARRAY(GameObject_test, char, name, GAMEOBJECT_NAME_MAX, AS_REFLECT_FORMAT_NONE) \
	/*Transform*/\
	AS_REFLECT_ENTRY_ARRAY(GameObject_test, float, position, 3, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_ARRAY(GameObject_test, float, rotationEuler, 3, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_ARRAY(GameObject_test, float, scale, 3, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_ARRAY(GameObject_test, float, velocity, 3, AS_REFLECT_FORMAT_NONE) \
	/*Parent/Child Heirarchy*/\
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, asHandle_t, parentHndl, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, asHandle_t, nextSiblingHndl, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, asHandle_t, firstChildHndl, AS_REFLECT_FORMAT_NONE) \
	/*Render Mesh*/\
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, asResourceFileID_t, renderMeshFileID, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, asHandle_t, renderMeshHandle, AS_REFLECT_FORMAT_NONE) \
	/*Custom Gameplay Data*/\
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, GameObjectClassesIndex_t, objTypeIdx, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_NOLOAD_SINGLE(GameObject_test, void*, pGameplayData, AS_REFLECT_FORMAT_NONE) \
	/*Doot*/\
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, int32_t, myInteger, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, float, myFloat, AS_REFLECT_FORMAT_NONE) \
	AS_REFLECT_ENTRY_SINGLE(GameObject_test, uint64_t, myUnsigned64BitInt, AS_REFLECT_FORMAT_NONE) \
)
REFLECT_MACRO_GameObject_test

void reflectTest();