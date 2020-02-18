#pragma once

#ifdef __cplusplus
#if __cplusplus < 201703L /*C++ 2020+ should be compatible*/
#error astrengine Reflection System Implimentation Requires 1999 ISO C Designated Initializers!
#endif
#endif

/**
* @file
* @brief Type reflection data generation for astrengine's data reflection system
*/

#include <stdint.h>
#include <stddef.h>

#include "../asCommon.h"

typedef struct {
	const uint16_t dataSize;
	const uint16_t dataOffset;
	const char typeName[30];
	const char varName[30];
} asReflectEntry;

typedef struct {
	const char name[48];
	const uint16_t binarySize;
	uint16_t entryCount;
	const asReflectEntry* data;
} asReflectContainer;

#ifdef AS_REFLECT_STRUCT
#undef AS_REFLECT_STRUCT
#endif
/**Declare a struct (name as string must be less than 48 bytes)*/
#define AS_REFLECT_STRUCT(_name, _content) {.name = #_name, .binarySize = (uint16_t)sizeof(_name), .entryCount = 0, .data = (asReflectEntry[]){_content {0,0,"",""}}};

#ifdef AS_REFLECT_ENTRY_SINGLE
#undef AS_REFLECT_ENTRY_SINGLE
#endif
/**Declare a struct (name and type as string must be less than 30 bytes)*/
#define AS_REFLECT_ENTRY_SINGLE(_structName, _type, _name, _formatOptions) {(uint16_t)sizeof(_type), (uint16_t)offsetof(_structName, _name), #_type, #_name},

#ifdef AS_REFLECT_ENTRY_ARRAY
#undef AS_REFLECT_ENTRY_ARRAY
#endif
/**Declare a struct (name and (type + "[]") as string must be less than 30 bytes)*/
#define AS_REFLECT_ENTRY_ARRAY(_structName, _type, _name, _count, _formatOptions) {(uint16_t)(sizeof(_type) * _count), (uint16_t)offsetof(_structName, _name[0]), #_type"[]", #_name},

#ifdef AS_REFLECT_ENTRY_NOLOAD_SINGLE
#undef AS_REFLECT_ENTRY_NOLOAD_SINGLE
#endif
#define AS_REFLECT_ENTRY_NOLOAD_SINGLE(_structName, _type, _name)

#ifdef AS_REFLECT_ENTRY_NOLOAD_ARRAY
#undef AS_REFLECT_ENTRY_NOLOAD_ARRAY
#endif
#define AS_REFLECT_ENTRY_NOLOAD_ARRAY(_structName, _type, _name, _count)

#ifdef AS_REFLECT_ENTRY_UNION
#undef AS_REFLECT_ENTRY_UNION
#endif
#define AS_REFLECT_ENTRY_UNION(_content)

#ifdef AS_REFLECT_ENTRY_COMPOUND
#undef AS_REFLECT_ENTRY_COMPOUND
#endif
#define AS_REFLECT_ENTRY_COMPOUND(_content)

/*Get amount of struct members*/
ASEXPORT int asReflectContainerMemberCount(const asReflectContainer* pContainer);