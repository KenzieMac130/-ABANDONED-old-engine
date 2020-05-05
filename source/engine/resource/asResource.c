#include "asResource.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)
#include <sys/stat.h>
#endif

#include <SDL_filesystem.h>

#include "stb/stb_ds.h"
#include "mattias/strpool.h"
/*Todo: Rewrite this mess*/

char resourceDir[1024];

#define RESOURCE_DIR_START "resources/"
const char* _sterilizeResourcePath(char* tmpBuff, size_t size)
{
	/*Remove Windows slashes*/
	for (size_t i = 0; i < size; i++)
	{
		if (tmpBuff[i] == '\\')
			tmpBuff[i] = '/';
	}
	/*Make relative to directory*/
	const char* buffStart = strstr(tmpBuff, RESOURCE_DIR_START);
	if (!buffStart)
		buffStart = tmpBuff;
	/*Remove beginning slash*/
	if (buffStart[0] == '/')
		buffStart++;
	return buffStart;
}

ASEXPORT asResourceFileID_t asResource_FileIDFromRelativePath(const char* pPath, size_t size)
{
	ASASSERT(size <= 1024);
	char tmpBuff[1024];
	memset(tmpBuff, 0, 1024);
	memcpy(tmpBuff, pPath, size);
	const char* buffStart = _sterilizeResourcePath(tmpBuff, size);
	return (asResourceFileID_t)asHashBytes64_xxHash(buffStart, strlen(buffStart));
}

ASEXPORT const char* asResource_GetResourceFolderPath()
{
	if (resourceDir[0] == '\0')
	{
		char* binDir = SDL_GetBasePath();
		if (!binDir)
		{
			asFatalError("SDL Base Path is Null?!!?");
		}

		asDebugLog("Base Path: %s", binDir);
		if (strlen(binDir) > 756)
		{
			asFatalError("Executable path exceeds 756 bytes, Please change install directory...");
		}
		memset(resourceDir, 0, 1024);
		memcpy(resourceDir, binDir, strlen(binDir));
		SDL_free(binDir);
		if (strstr(resourceDir, "bin") == NULL)
		{
			asFatalError("Please Relocate Executable to A \"bin\" Directory...");
		}
		const char* tmp;
		while ((tmp = strstr(resourceDir, "bin")) != NULL)
			resourceDir[tmp - resourceDir] = '\0';
		strncat(resourceDir, RESOURCE_DIR_START, 100);
		for (size_t i = 0; i < strlen(resourceDir); i++)
		{
			if (resourceDir[i] == '\\')
				resourceDir[i] = '/';
		}
	}
	return resourceDir;
}

ASEXPORT const char* asResource_GetBinFolderPath()
{
	return SDL_GetBasePath();
}

/*Loader*/

/*Resource Lookup*/
struct fInfo { STRPOOL_U64 nameId; int64_t start; int64_t size; };
struct
{
	struct { asResourceFileID_t key; struct fInfo value; }*fileMap;
	strpool_t strPool;
} resourceLookupPool;

ASEXPORT asResults asResourceLoader_Open(asResourceLoader_t * loader, asResourceFileID_t id)
{
	size_t resourceIndex = hmgeti(resourceLookupPool.fileMap, id);
	if (resourceIndex == SIZE_MAX)
	{
		asDebugWarning("Unknown Resource ID: %llx (Not registered in Manifest?)", id);
		return AS_FAILURE_FILE_NOT_FOUND;
	}

	const char* relativeName = strpool_cstr(&resourceLookupPool.strPool,
		resourceLookupPool.fileMap[resourceIndex].value.nameId);
	if (!relativeName)
	{
		return AS_FAILURE_UNKNOWN;
	}
	loader->_buffSize = resourceLookupPool.fileMap[resourceIndex].value.size;
	loader->_buffOffset = resourceLookupPool.fileMap[resourceIndex].value.start;

	/*create full filepath*/
	char fileName[1024];
	strncpy(fileName, resourceDir, 1024);
	strncat(fileName, relativeName, strlen(relativeName));
	loader->_fileHndl = fopen(fileName, "rb");
	if (!loader->_fileHndl)
		return AS_FAILURE_FILE_INACCESSIBLE;
	if (loader->_buffSize < 0)
	{
		fseek(loader->_fileHndl, 0, SEEK_END);
		loader->_buffSize = ftell(loader->_fileHndl);
	}

	fseek(loader->_fileHndl, (long)loader->_buffOffset, SEEK_SET);
	return AS_SUCCESS;
}

ASEXPORT asResults asResourceLoader_OpenByPath(asResourceLoader_t* loader, asResourceFileID_t* pId, char* path, size_t pathLength)
{
	asDebugLog("Opening Named Resource: %.*s", (int)pathLength, path);
	asResourceFileID_t resID = asResource_FileIDFromRelativePath(path, pathLength);
	*pId = resID;
	return asResourceLoader_Open(loader, resID);
}

ASEXPORT void asResourceLoader_Close(asResourceLoader_t* loader)
{
	fclose(loader->_fileHndl);
}

ASEXPORT size_t asResourceLoader_GetContentSize(asResourceLoader_t * loader)
{
	return loader->_buffSize > 0? (size_t)loader->_buffSize : 0;
}

ASEXPORT asResults asResourceLoader_SetReadPoint(asResourceLoader_t * loader, size_t pos)
{
	fseek(loader->_fileHndl, (long)(loader->_buffOffset + pos), SEEK_SET);
	return AS_SUCCESS;
}

ASEXPORT asResults asResourceLoader_Read(asResourceLoader_t * loader, size_t size, void * buff)
{
	fread(buff, size, 1, loader->_fileHndl);
	return AS_SUCCESS;
}

ASEXPORT asResults asResourceLoader_ReadAll(asResourceLoader_t * loader, size_t size, void * buff)
{
	fseek(loader->_fileHndl, (long)loader->_buffOffset, SEEK_SET);
	fread(buff, loader->_buffSize, 1, loader->_fileHndl);
	return AS_SUCCESS;
}

/*Resource data mapping*/

struct _resourceDat
{
	asResourceDataMapping_t mapping;
	int32_t references;
	asHash32_t type;
};
struct { asResourceFileID_t key; struct _resourceDat value; }*resMap = NULL;

ASEXPORT void asResource_Create(asResourceFileID_t id, asResourceDataMapping_t mapping, asHash32_t type, int32_t initialRefs)
{
	struct _resourceDat data;
	data.mapping = mapping;
	data.references = initialRefs;
	data.type = type;
	hmput(resMap, id, data);
}

ASEXPORT asResourceDataMapping_t asResource_GetExistingDataMapping(asResourceFileID_t id, asHash32_t requiredType)
{
	struct _resourceDat resource = hmget(resMap, id);
	if (resource.type == requiredType)
	{
		return resource.mapping;
	}
	return (asResourceDataMapping_t){ .hndl = asHandle_Invalidate(), .ptr = NULL };
}

/*Deletion*/

struct { asHash32_t key; asResourceDataMapping_t* value; }*resDeleteLists = NULL;

ASEXPORT asResourceType_t asResource_RegisterType(const char* typeName, size_t strSize)
{
	asHash32_t type = asHashBytes32_xxHash(typeName, strSize);
	asResourceDataMapping_t* arr = NULL;
	arrsetcap(arr, 64);
	hmput(resDeleteLists, type, arr);
	return type;
}

ASEXPORT size_t asResource_GetDeletionQueue(asResourceType_t type, size_t* pCount, asResourceDataMapping_t ** ppMappings)
{
	asResourceDataMapping_t* map = hmget(resDeleteLists, type);
	ASASSERT(map);
	*ppMappings = map;
	if (pCount) { *pCount = arrlen(map); }
	return arrlen(map);
}

ASEXPORT void asResource_ClearDeletionQueue(asResourceType_t type)
{
	asResourceDataMapping_t* map = hmget(resDeleteLists, type);
	ASASSERT(map);
	arrdeln(map, 0, arrlen(map));
}

ASEXPORT void asResource_GetFileName(asResourceFileID_t id, const char ** ppName, int32_t * pNameLength)
{
	size_t resourceIndex = hmgeti(resourceLookupPool.fileMap, id);
	*ppName = strpool_cstr(&resourceLookupPool.strPool,
		resourceLookupPool.fileMap[resourceIndex].value.nameId);
	*pNameLength = (int32_t)strlen(*ppName);
}

ASEXPORT void asResource_IncrimentReferences(asResourceFileID_t id, uint32_t addRefCount)
{
	struct _resourceDat* res;
	if (res = &hmget(resMap, id))
	{
		if (res->references >= 0)
			res->references += addRefCount;
	}
}

ASEXPORT void asResource_DeincrimentReferences(asResourceFileID_t id, uint32_t subRefCount)
{
	struct _resourceDat* res;
	if (res = &hmget(resMap, id))
	{
		if (res->references >= 0)
		{
			if ((res->references -= subRefCount) <= 0)
			{
				asResourceDataMapping_t* map;
				if (map = hmget(resDeleteLists, res->type))
				{
					arrput(map, res->mapping);
				}
				res->references--; /*prevent multiple removals*/
			}
		}
	}
}

void _generateResourceEntires(const char* manifestPath)
{
	asCfgFile_t* manifest = asCfgLoad(manifestPath);
	{
		struct fInfo fileInfo;
		char* override = NULL;
		size_t overrideSize;
		char* name = NULL;
		size_t nameSize;
		asResourceFileID_t id;
		/*files*/
		asCfgOpenSection(manifest, "files");
		while (!asCfgGetNextProp(manifest, &override, &name))
		{
			nameSize = strlen(name);
			overrideSize = strlen(override);
			if (nameSize > 256)
				continue;
			char fullPath[1024];
			memset(fullPath, 0, 1024);
			strncpy(fullPath, resourceDir, 1023);
			strncat(fullPath, name, nameSize);
			fileInfo.size = -1; /*Undefined size, determined using ftell*/

			/*Add Name Override*/
			char* finalName = name;
			size_t finalNameSize = nameSize;
			if (strncmp(override, "_PATH_", overrideSize))
			{
				finalName = override;
				finalNameSize = overrideSize;
				asDebugLog("RESOURCE OVERRIDE! %s = %s", override, name);
			}

			fileInfo.nameId = strpool_inject(&resourceLookupPool.strPool, name, (int)nameSize);
			fileInfo.start = 0; /*Stray files always start at byte 0*/
			id = asResource_FileIDFromRelativePath(finalName, finalNameSize);
			hmput(resourceLookupPool.fileMap, id, fileInfo);
		}
		/*packages*/
		//asCfgOpenSection(manifest, "aspak");
		//while (asCfgGetNextProp(manifest, override, name))
		//{
		//	/*Todo: Future package system*/
		//}
	}
}

/*Manager*/

ASEXPORT void asInitResource()
{
	/*Get Resource Path*/
	{
		asResource_GetResourceFolderPath();
		asDebugLog("Resource Path: %s", resourceDir);
	}

	/*Get Manifest Path*/
	char manifestPath[1024];
	memset(manifestPath, 0, 1024);
	strncpy(manifestPath, resourceDir, 1024-14);
	strncat(manifestPath, "resources.cfg", 14);
	asDebugLog("Resource Manifest: %s", manifestPath);

	/*Initialize Manager*/
	struct _resourceDat defRes;
	defRes.mapping.hndl = asHandle_Invalidate();
	defRes.mapping.ptr = NULL;
	defRes.references = 0;
	defRes.type = 0;
	hmdefault(resMap, defRes);
	strpool_init(&resourceLookupPool.strPool, &strpool_default_config);
	_generateResourceEntires(manifestPath);
}

ASEXPORT void asShutdownResource()
{
	hmfree(resMap);
	hmfree(resourceLookupPool.fileMap);
	strpool_term(&resourceLookupPool.strPool);
}