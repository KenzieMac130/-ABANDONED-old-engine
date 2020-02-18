#pragma once

/**
* @file
* @brief Type definition for astrengine's data reflection system
*/

#include <stdint.h>

typedef enum {
	AS_REFLECT_FORMAT_NONE = 0x0000,
	AS_REFLECT_FORMAT_MAX = UINT32_MAX
} asReflectFormatOptions;

#ifdef AS_REFLECT_STRUCT
#undef AS_REFLECT_STRUCT
#endif
#define AS_REFLECT_STRUCT(_name, _content) typedef struct {_content} _name;

#ifdef AS_REFLECT_ENTRY_SINGLE
#undef AS_REFLECT_ENTRY_SINGLE
#endif
#define AS_REFLECT_ENTRY_SINGLE(_structName, _type, _name, _formatOptions) _type _name;

#ifdef AS_REFLECT_ENTRY_ARRAY
#undef AS_REFLECT_ENTRY_ARRAY
#endif
#define AS_REFLECT_ENTRY_ARRAY(_structName, _type, _name, _count, _formatOptions) _type _name[_count];

#ifdef AS_REFLECT_ENTRY_NOLOAD_SINGLE
#undef AS_REFLECT_ENTRY_NOLOAD_SINGLE
#endif
#define AS_REFLECT_ENTRY_NOLOAD_SINGLE(_structName, _type, _name) _type _name;

#ifdef AS_REFLECT_ENTRY_NOLOAD_ARRAY
#undef AS_REFLECT_ENTRY_NOLOAD_ARRAY
#endif
#define AS_REFLECT_ENTRY_NOLOAD_ARRAY(_structName, _type, _name, _count) _type _name[_count];

#ifdef AS_REFLECT_ENTRY_UNION
#undef AS_REFLECT_ENTRY_UNION
#endif
#define AS_REFLECT_ENTRY_UNION(_content) union {_content};

#ifdef AS_REFLECT_ENTRY_COMPOUND
#undef AS_REFLECT_ENTRY_COMPOUND
#endif
#define AS_REFLECT_ENTRY_COMPOUND(_content) struct {_content};