#ifndef _ASBINDLESSTEXTUREPOOL_H_
#define _ASBINDLESSTEXTUREPOOL_H_

#include "../engine/common/asCommon.h"
#include "../engine/renderer/asRendererCore.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t asBindlessTextureIndex;

#define AS_MAX_POOLED_TEXTURES 1024

#define AS_BINDING_TEXTURE_POOL 200
#define AS_DESCSET_TEXTURE_POOL 0

ASEXPORT asResults asTexturePoolAddFromHandle(const asTextureHandle_t handle, asBindlessTextureIndex* pTextureIndex);

ASEXPORT asResults asTexturePoolRelease(asBindlessTextureIndex textureIndex);

ASEXPORT asResults asInitTexturePool();

ASEXPORT asResults asTexturePoolUpdate();

ASEXPORT void _asTexturePoolDebug();

ASEXPORT asResults asShutdownTexturePool();

#if ASTRENGINE_VK /*Vulkan Stuff*/
ASEXPORT void asVkGetTexturePoolDescSetLayout(void* pDest);
#endif

ASEXPORT asResults asTexturePoolBindCmd(asGfxAPIs apiValidate, void* pCmdBuff, void* pLayout, int frame);

#ifdef __cplusplus
}
#endif
#endif