#ifndef _ASFIRSTPERSONCAMERA_H_
#define _ASFIRSTPERSONCAMERA_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "../common/asCommon.h"

ASEXPORT void asGameUtils_FlyCamera(float deltaTime,
	vec3 movement,
	float xLook, float yLook,
	float* pYaw, float* pPitch,
	float lookSensitivity, float speed,
	vec3 cameraPos, quat cameraRot);

#ifdef __cplusplus
}
#endif

#endif