#include "asDebugDraw.h"

#include <SDL_thread.h>

asDebugDrawLineData* lineList = NULL;
SDL_mutex* drawAccessMutex = NULL;

void _drawAtExit(void)
{
	if (lineList)
	{
		arrfree(lineList);
	}

	if (drawAccessMutex)
	{
		SDL_UnlockMutex(drawAccessMutex);
		SDL_DestroyMutex(drawAccessMutex);
		drawAccessMutex = NULL;
	}
}

int secureMutex()
{
	if (!drawAccessMutex) /*Assumes first call to debug draw happens before thread management system is launched*/
	{
		drawAccessMutex = SDL_CreateMutex();
		atexit(_drawAtExit);
		ASASSERT(drawAccessMutex);
	}

	return SDL_LockMutex(drawAccessMutex) == 0;
}

void unlockMutex()
{
	SDL_UnlockMutex(drawAccessMutex);
}

ASEXPORT asResults _asDebugDrawSecureLists()
{
	secureMutex();
	return AS_SUCCESS;
}

ASEXPORT void _asDebugDrawUnlockLists()
{
	unlockMutex();
}

ASEXPORT size_t _asDebugDrawGetLineList(asDebugDrawLineData** ppLineList)
{
	if (ppLineList) { *ppLineList = lineList; }
	return arrlen(lineList);
}

ASEXPORT void _asDebugDrawResetLineList()
{
	arrsetlen(lineList, 0);
}

/*Drawing*/
void drawLine(vec3 start, vec3 end, float thickness, vec4 color)
{
	asDebugDrawLineData result;
	glm_vec3_copy(start, result.start);
	glm_vec3_copy(end, result.end);
	result.color[0] = (int8_t)(color[0] * 255);
	result.color[1] = (int8_t)(color[1] * 255);
	result.color[2] = (int8_t)(color[2] * 255);
	result.color[3] = (int8_t)(color[3] * 255);
	result.thickness = thickness;
	arrput(lineList, result);
}

ASEXPORT asResults asDebugDrawLine3D(vec3 start, vec3 end, float thickness, vec4 color)
{
	if (secureMutex())
	{
		drawLine(start, end, thickness, color);
		unlockMutex();
	}
	return AS_SUCCESS;
}
