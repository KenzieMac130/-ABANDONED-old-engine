#ifndef _ASMANUALSERIALLIST_H_
#define _ASMANUALSERIALLIST_H_

#ifdef __cplusplus 
extern "C" {
#endif

/**
* @file
* @brief Manually fill out a list of properties for serialization
*/

#include <stdint.h>
#include <stddef.h>

#include "../asCommon.h"
#include "asReflectImpliment.h"

typedef struct {
    size_t fieldCapacity;
    size_t dataSize;
    size_t dataCount;
    asReflectContainer reflectContainer;
    unsigned char* pData;
} asManualSerialList;

ASEXPORT asResults asManualSerialListCreate(asManualSerialList* pSerialList, const char* structName, size_t initialPropCount, size_t dataSize, size_t dataCount);

ASEXPORT asResults asManualSerialListAddProperties(asManualSerialList* pSerialList, const char* type, const char* name, unsigned char* pSrc, size_t srcSize, size_t stride, size_t count);

ASEXPORT asResults asManualSerialListAddProperty(asManualSerialList* pSerialList, const char* type, const char* name, unsigned char* pSrc, size_t srcSize);

ASEXPORT asResults asManualSerialListFree(asManualSerialList* pSerialList);

#ifdef __cplusplus
}
#endif
#endif