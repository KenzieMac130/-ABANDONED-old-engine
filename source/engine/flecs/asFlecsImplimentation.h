#ifndef _ASFLECSIMPLIMENTATION_H_
#define _ASFLECSIMPLIMENTATION_H_

#include "../common/asCommon.h"
#ifdef __cplusplus
extern "C" {
#endif 

#if ASTRENGINE_FLECS
#include "../thirdparty/flecs/include/flecs.h"
ASEXPORT bool asIsFlecsEnabled();

ASEXPORT asResults asInitFlecs(int argc, char** argv);

ASEXPORT asResults asShutdownFlecs();

ASEXPORT asResults asUpdateFlecs(float deltaTime);

ASEXPORT ecs_world_t* asEcsGetWorldPtr();
#endif

#ifdef __cplusplus
}
#endif
#endif