#ifndef _ASDEBUGDRAW_H_
#define _ASDEBUGDRAW_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

typedef struct {
	vec3 start;
	vec3 end;
	float thickness;
	union {
		uint8_t color[4];
		uint32_t color32;
	};
} asDebugDrawLineData;

/*Secure the debug lists for read access (DO NOT TOUCH UNLESS YOU KNOW WHAT YOU ARE DOING)*/
ASEXPORT asResults _asDebugDrawSecureLists();
ASEXPORT void _asDebugDrawUnlockLists();

/*Get Debug Draw Line Lists (DO NOT USE WHILE LOGGING!!!)*/
ASEXPORT size_t _asDebugDrawGetLineList(asDebugDrawLineData** ppLineList);
ASEXPORT void _asDebugDrawResetLineList();

/*Draw a 2D Line*/
ASEXPORT asResults asDebugDrawLine3D(vec3 start, vec3 end, float thickness, vec4 color);

#ifdef __cplusplus
}
#endif
#endif