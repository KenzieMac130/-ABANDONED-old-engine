/**
* @file
* @brief Type reflection data generation for astrengine's data reflection system
*/

#ifndef _ASREFLECTIOBINARY_H_
#define _ASREFLECTIOBINARY_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "../asCommon.h"

ASEXPORT size_t asReflectGetBinarySize(const struct asReflectContainer* pReflectData, uint32_t srcCount);

ASEXPORT asResults asReflectSaveToBinary(
    unsigned char* pDest,
    const size_t destSize,
    const struct asReflectContainer* pReflectData,
    const unsigned char* pSrc,
    const uint32_t srcCount);

ASEXPORT asResults asReflectLoadFromBinary(
    unsigned char* pDest,
    const size_t destSize,
    const uint32_t destCount,
    const struct asReflectContainer* pDestReflectData,
    const unsigned char* pSrc,
    const size_t srcSize,
    uint32_t* pSrcCount,
    struct asReflectContainer** ppOutSrcReflectData);

#ifdef __cplusplus
}
#endif
#endif