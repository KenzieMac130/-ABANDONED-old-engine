#pragma once

#include "asCommon.h"
#ifdef __cplusplus
extern "C" {
#endif 

/**
* @brief Initializes the resource manager
* @warning The engine ignite should handle this for you
*/
ASEXPORT void asInitResource(const char * manifest);

/**
* @brief Shutdown the resource manager
* @warning The engine shutdown should handle this for you
*/
ASEXPORT void asShutdownResource();

/**
* @brief a Resource File ID
*/
typedef uint64_t asResourceFileID_t;

/**
* @brief a Handle to a resource
*/
typedef asHandle_t asResourceHandle_t;

/**
* @brief a Resource type
*/
typedef asHash32_t asResourceType_t;

/**
* @brief Creates a asResourceFileID_t from the path relative to the assets folder
*/
ASEXPORT asResourceFileID_t asResource_FileIDFromRelativePath(const char* pPath, size_t size);

/**
* @brief an opaque Resource File Loader Context (read only)
*/
typedef struct
{
	FILE* _fileHndl;
	int64_t _buffOffset;
	int64_t _buffSize;
} asResourceLoader_t;

/**
* @brief Open a resource file for reading (abstacts into the package system)
*/
ASEXPORT asResults asResourceLoader_Open(asResourceLoader_t* loader, asResourceFileID_t id);

/**
* @brief Release the file
*/
ASEXPORT void asResourceLoader_Close(asResourceLoader_t* loader);

/**
* @brief Get file contents size
*/
ASEXPORT size_t asResourceLoader_GetContentSize(asResourceLoader_t* loader);

/**
* @brief Set the read point
* similar to calling fseek() with an absolute offset
*/
ASEXPORT asResults asResourceLoader_SetReadPoint(asResourceLoader_t* loader, size_t pos);

/**
* @brief Read some data from the file
* similar to calling fread()
* @warning do not read further than the size of the resource, will result in crash/undifined behavior
*/
ASEXPORT asResults asResourceLoader_Read(asResourceLoader_t* loader, size_t size, void* buff);

/**
* @brief Read all data from the file
* similar to calling fread() on the whole file
* @warning buffer must be large enough to store all of the data, read functions must only be called once
*/
ASEXPORT asResults asResourceLoader_ReadAll(asResourceLoader_t* loader, size_t size, void* buff);

/**
* @brief Maps to data associated with a resource
*/
typedef struct
{
	asHandle_t hndl;
	void* ptr;
} asResourceDataMapping_t;

/**
* @brief Set the mapping for the data associated with the resource
*/
ASEXPORT void asResource_Create(asResourceFileID_t id, asResourceDataMapping_t mapping, asHash32_t type, int32_t initialRefs);

/**
* @brief Get a data handle for an already loaded resource
* Returns invalid handle if no handle is associated
*/
ASEXPORT asResourceDataMapping_t asResource_GetExistingDataMapping(asResourceFileID_t id, asHash32_t requiredType);

/**
* @brief Incriment the references to a resource
*/
ASEXPORT void asResource_IncrimentReferences(asResourceFileID_t id, uint32_t addRefCount);

/**
* @brief Deincriment the references to a resource
*/
ASEXPORT void asResource_DeincrimentReferences(asResourceFileID_t id, uint32_t subRefCount);

/**
* @brief Register type
*/
ASEXPORT asResourceType_t asResource_RegisterType(const char* typeName, size_t strSize);
/**
* @brief Get deletion queue by type
*/
ASEXPORT size_t asResource_GetDeletionQueue(asResourceType_t type, asResourceDataMapping_t** ppMappings);

/**
* @brief Clear deletion queue by type
*/
ASEXPORT void asResource_ClearDeletionQueue(asResourceType_t type);

#ifdef __cplusplus
}
#endif