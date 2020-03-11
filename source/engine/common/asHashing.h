#ifndef _ASHASHING_H_
#define _ASHASHING_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

/*Hashing*/

/**
* @brief a 64 bit hash type
*/
typedef uint64_t asHash64_t;

/**
* @brief Hash bytes to a asHash64_t using the xxHash library
*/
ASEXPORT asHash64_t asHashBytes64_xxHash(const void *pBytes, size_t size);

/**
* @brief a 32 bit hash type
*/
typedef uint32_t asHash32_t;

/**
* @brief Hash bytes to a asHash32_t using the xxHash library
*/
ASEXPORT asHash32_t asHashBytes32_xxHash(const void *pBytes, size_t size);

#ifdef __cplusplus
}
#endif
#endif