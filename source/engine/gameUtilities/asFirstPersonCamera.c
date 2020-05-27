#include "asFirstPersonCamera.h"

ASEXPORT void asGameUtils_FirstPersonCamera(float deltaTime, 
	vec3 movement,
	float xLook, float yLook,
	float* pYaw, float* pPitch,
	float lookSensitivity, float speed,
	vec3 cameraPos, quat cameraRot)
{
	/*Camera Look*/
	*pYaw -= xLook * lookSensitivity;
	*pPitch -= yLook * lookSensitivity;
	if (*pPitch > 89.999f)
		*pPitch = 89.999f;
	if (*pPitch < -89.999f)
		*pPitch = -89.999f;

	quat rotYaw = ASQUAT_IDENTITY;
	quat rotPitch = ASQUAT_IDENTITY;
	glm_quatv(rotYaw, glm_rad(*pYaw), (vec3)ASVEC3_UP);
	glm_quatv(rotPitch, glm_rad(*pPitch), (vec3)ASVEC3_RIGHT);
	glm_quat_mul(rotYaw, rotPitch, cameraRot);

	/*Camera Movement*/
	vec3 forward = ASVEC3_FORWARD;
	vec3 right = ASVEC3_RIGHT;
	vec3 up = ASVEC3_UP;
	glm_quat_rotatev(cameraRot, forward, forward);
	glm_quat_rotatev(cameraRot, right, right);
	glm_cross(forward, right, up);
	glm_vec3_scale(forward, (speed * deltaTime * movement[2]), forward);
	glm_vec3_scale(right, (speed * deltaTime * movement[0]), right);
	glm_vec3_scale(up, (speed * deltaTime * movement[1]), up);
	glm_vec3_add(cameraPos, forward, cameraPos);
	glm_vec3_add(cameraPos, right, cameraPos);
	glm_vec3_add(cameraPos, up, cameraPos);
}
