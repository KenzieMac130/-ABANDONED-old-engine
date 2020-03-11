#ifndef _ASINSTANCERENDERER_H_
#define _ASINSTANCERENDERER_H_

#include "../engine/common/asCommon.h"
#include "../engine/renderer/asRendererCore.h"
#ifdef __cplusplus
extern "C" {
#endif 
/**
* @file
* @brief The high level instance renderer allows scene/entity/object managent to submit instances
* which are rendered using their respective materials, geometry with lighting and view transforms
* emphasis on instance renderer because this system attempts to bacth up as many calls as it can
*/

/**
* @brief Vertex format for regular/instanced models (no hardware skinning)
* Uses lossy compression, binormals are reconstructed in the vertex shader
* Only real limitations are with extremely large / 1024x pixel perfect UVs
* For large UVs I recomend repeating 0-1 or using the texture scale params
* Total compressed vertex size 32 bytes, the uncompresssed would be 60-76!
*/
typedef struct {
	float position[3]; /**< Positions: (XYZ) AS_COLORFORMAT_RGB32_SFLOAT*/
	uint32_t normals; /**< Normals: (ZYX) AS_COLORFORMAT_B10G11R11_UFLOAT (Bits Compressed)*/
	uint32_t tangents; /**< Tangents: (XYZ, Tan-W Sign, Nrm-Z Sign) AS_COLORFORMAT_R10G10B10A2_UNORM (Bits Compressed)*/
	uint16_t uv0[2]; /**< UV 0: (UV) AS_COLORFORMAT_RG16_SFLOAT*/
	uint16_t uv1[2]; /**< UV 1: (UV) AS_COLORFORMAT_RG16_SFLOAT*/
	uint8_t rgba[4]; /**< Colors: (RGBA) AS_COLORFORMAT_RGBA8_UNORM*/
} asVertex_Generic_t;

#ifdef __cplusplus
}
#endif
#endif