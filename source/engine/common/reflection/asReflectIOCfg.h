/**
* @file
* @brief Type reflection data generation for astrengine's data reflection system
*/

#include <stdint.h>
#include <stddef.h>

#include "../asCommon.h"
#include "../asConfigFile.h"

/*Only supports partial saving (char[], int types, float, double)*/
ASEXPORT asResults asReflectSaveToCfg(
    asCfgFile_t* pDest,
    const struct asReflectContainer* pReflectData,
    const unsigned char* pSrc);

/*Only supports partial loading (char[], int types, float, double)*/
ASEXPORT asResults asReflectLoadFromCfg(
    unsigned char* pDest,
    const size_t destSize,
    const struct asReflectContainer* pDestReflectData,
    asCfgFile_t* pSrc);