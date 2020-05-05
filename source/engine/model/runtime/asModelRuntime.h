#ifndef _ASMODELRUNTIME_H_
#define _ASMODELRUNTIME_H_

#include "../engine/common/asCommon.h"
#include "../engine/renderer/asRendererCore.h"
#include "../thirdparty/cglm/cglm.h"
#ifdef __cplusplus
extern "C" {
#endif 

#include "engine/common/reflection/asReflectDefine.h"

/*------------------------------------VERTEX------------------------------------*/
/**
* @brief Vertex format for common models
* position: vertex position in model space (XYZ) AS_COLORFORMAT_RGB32_SFLOAT
* normal: packed normals: (ZYX) AS_COLORFORMAT_B10G11R11_UFLOAT
* tangent: packed tangent XYZ with sign stored in W (WXYZ): AS_COLORFORMAT_A2R10G10B10_SNORM
* uv0: packed texture coordinate 0 (XY): AS_COLORFORMAT_RG16_SFLOAT
* uv1: packed texture coordinate 1 (XY): AS_COLORFORMAT_RG16_SFLOAT
* rgba: vertex color (RGBA): AS_COLORFORMAT_RGBA8_UNORM
* boneIdx: bone indices (4 bones) (XYZW): AS_COLORFORMAT_RGBA16_UINT
* boneWeight: bone weights (4 bones) (XYZW): AS_COLORFORMAT_RGBA8_UNORM
*/
#define REFLECT_MACRO_Vertex_Generic AS_REFLECT_STRUCT(asVertexGeneric,\
	AS_REFLECT_ENTRY_ARRAY(asVertexGeneric, float, position, 3, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asVertexGeneric, uint32_t, normal, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asVertexGeneric, uint32_t, tangent, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asVertexGeneric, uint32_t, uv0, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_SINGLE(asVertexGeneric, uint32_t, uv1, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_ARRAY(asVertexGeneric, uint8_t, color, 4, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_ARRAY(asVertexGeneric, uint16_t, boneIdx, 4, AS_REFLECT_FORMAT_NONE)\
	AS_REFLECT_ENTRY_ARRAY(asVertexGeneric, uint8_t, boneWeight, 4, AS_REFLECT_FORMAT_NONE)\
)

REFLECT_MACRO_Vertex_Generic

ASEXPORT void asVertexGeneric_encodePosition(asVertexGeneric* pVertex, const vec3 pos);
ASEXPORT void asVertexGeneric_encodeNormal(asVertexGeneric* pVertex, const vec3 normal);
ASEXPORT void asVertexGeneric_encodeTangent(asVertexGeneric* pVertex, const vec3 tangent, const int sign);
ASEXPORT void asVertexGeneric_encodeUV(asVertexGeneric* pVertex, int uvSet, const vec2 uv);
ASEXPORT void asVertexGeneric_encodeColor(asVertexGeneric* pVertex, const uint8_t rgba[4]);
ASEXPORT void asVertexGeneric_encodeBoneIds_Mult(asVertexGeneric* pVertex, const uint16_t ids[4]);
ASEXPORT void asVertexGeneric_encodeBoneWeights_Mult(asVertexGeneric* pVertex, const float weight[4]);

/*------------------------------------SUBMESH------------------------------------*/

#ifdef __cplusplus
}
#endif
#endif