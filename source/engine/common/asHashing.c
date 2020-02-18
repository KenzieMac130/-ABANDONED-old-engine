#include "asHashing.h"

/*Hashing*/
#define XXH_STATIC_LINKING_ONLY
#define XXHASH_IMPLEMENTATION
#include "xxhash/xxHash.h"

ASEXPORT asHash64_t asHashBytes64_xxHash(const void *pBytes, size_t size)
{
	return (asHash64_t)XXH64(pBytes, size, 0);
}

ASEXPORT asHash32_t asHashBytes32_xxHash(const void *pBytes, size_t size)
{
	return (asHash32_t)XXH32(pBytes, size, 0);
}