#ifndef _ASTEXTUREFROMKTX_H_
#define _ASTEXTUREFROMKTX_H_

#include "../common/asCommon.h"
#include "asRendererCore.h"
#ifdef __cplusplus
extern "C" {
#endif 

/**
* @brief Fills out the data for a texture description from ktx (usually loaded from a file)
* user is responsible for freeing ktx data
*/
ASEXPORT asResults asTextureDesc_FromKtxData(asTextureDesc_t* pOut, void* pKtxBlob, size_t blobSize);

ASEXPORT void asTextureDesc_FreeKtxData(asTextureDesc_t* pOut);

#ifdef __cplusplus
}
#endif
#endif