#include "asCommon.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SDL_timer.h>

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"
#define  STRPOOL_IMPLEMENTATION
#include "mattias/strpool.h"
#define HASHTABLE_IMPLEMENTATION
#include "mattias/hashtable.h"

/*Errors*/

ASEXPORT void asError(const char* msg)
{
#ifdef _WIN32
	MessageBoxA(0, (LPCSTR)msg, (LPCSTR)"astrengine Error", MB_OK);
#endif
}

ASEXPORT void asFatalError(const char* msg)
{
	asError(msg);
	exit(1);
}