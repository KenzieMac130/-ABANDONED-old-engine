#include "asModelRuntime.h"

#include "tiny_imageFormat/encodehelpers.h"

ASEXPORT void asVertexGeneric_encodePosition(asVertexGeneric* pVertex, const vec3 pos)
{
	float* pPos = (float*)pVertex->position;
	memcpy(pPos, pos, sizeof(float) * 3);
}

ASEXPORT void asVertexGeneric_encodeNormal(asVertexGeneric* pVertex, const vec3 normal)
{
	struct encoding {
		union {
			struct {
				unsigned int b : 10;
				unsigned int g : 11;
				unsigned int r : 11;
			};
			uint32_t full;
		};
	};
	vec3 normalizedNormal = {
		normal[0] * 0.5f + 0.5f,
		normal[1] * 0.5f + 0.5f,
		normal[2] * 0.5f + 0.5f };
	struct encoding result;
	result.full = 0;
	result.b = TinyImageFormat_FloatToUFloat10AsUint(normalizedNormal[2]);
	result.g = TinyImageFormat_FloatToUFloat11AsUint(normalizedNormal[1]);
	result.r = TinyImageFormat_FloatToUFloat11AsUint(normalizedNormal[0]);
	pVertex->normal = result.full;
}

ASEXPORT void asVertexGeneric_encodeTangent(asVertexGeneric* pVertex, const vec3 tangent, const int sign)
{
	struct encoding {
		union {
			struct {
				int a : 2;
				int r : 10;
				int g : 10;
				int b : 10;
			};
			uint32_t full;
		};
	};
	struct encoding result;
	result.full = 0;
	result.a = sign;
	result.r = (int)(tangent[0] * 511);
	result.g = (int)(tangent[1] * 511);
	result.b = (int)(tangent[2] * 511);
	pVertex->tangent = result.full;
}

ASEXPORT void asVertexGeneric_encodeUV(asVertexGeneric* pVertex, int uvSet, const vec2 uv)
{
	uint16_t* pUVSet = (uint16_t*)((uvSet == 0) ? &pVertex->uv0 : &pVertex->uv1);
	pUVSet[0] = TinyImageFormat_FloatToHalfAsUint(uv[0]);
	pUVSet[1] = TinyImageFormat_FloatToHalfAsUint(uv[1]);
}

ASEXPORT void asVertexGeneric_encodeColor(asVertexGeneric* pVertex, const uint8_t rgba[4])
{
	uint8_t* pColor = (uint8_t*)&pVertex->color;
	memcpy(pColor, rgba, 4);
}

ASEXPORT void asVertexGeneric_encodeBoneIds_Mult(asVertexGeneric* pVertex, const uint16_t ids[4])
{
	uint16_t* pIdx = (uint16_t*)pVertex->boneIdx;
	memcpy(pIdx, ids, sizeof(uint16_t) * 4);
}

ASEXPORT void asVertexGeneric_encodeBoneWeights_Mult(asVertexGeneric* pVertex, const float weight[4])
{
	uint8_t* pWeights = (uint8_t*)&pVertex->boneWeight;
	for (int i = 0; i < 4; i++) {
		pWeights[i] = (uint8_t)(weight[i] * 255);
	}
}