#include "ManifestBuild.h"

#include <engine/common/asCommon.h>
#include <cute/cute_files.h>
#define INI_IMPLEMENTATION
#include <mattias/ini.h>

typedef struct {
	int filesSection;
	const char* resourceDirectory;
	ini_t* pManifestIni;
} ResourceAddInfo;

void ResourceFileAdd(cf_file_t* pFile, void* pData)
{
	ResourceAddInfo* ctx = (ResourceAddInfo*)pData;
	/*Exclude Make File*/
	if (!strcmp(pFile->name, "resources.cfg"))
	{
		return;
	}

	/*Turn File Path into Relative*/
	char finalPath[1024];
	memset(finalPath, 0, 1024);
	char* foundPos = strstr(pFile->path, ctx->resourceDirectory); /*Find First Relative Pos*/
	ASASSERT(foundPos); /*This should always work*/
	foundPos += strlen(ctx->resourceDirectory); /*Make Relative*/
	while (*foundPos == '/' || *foundPos == '\\') { /*Remove Beginning Slashes*/
		foundPos++;
	}
	strncat(finalPath, foundPos, 1023);
	for (int i = 0; i < strlen(finalPath); i++) { /*Remove Windows Nasties*/
		if (finalPath[i] == '\\') {
			finalPath[i] = '/';
		}
	}

	/*Add Property*/
	ini_property_add(ctx->pManifestIni, ctx->filesSection, "_PATH_", 7, finalPath, strlen(finalPath));
}

int MergeRenameList(ini_t* pManifestIni, ini_t* pRenameListIni)
{
	int rlFilesSection = ini_find_section(pRenameListIni, "files", 6);
	int manFilesSection = ini_find_section(pManifestIni, "files", 6);

	for (int i = 0; i < ini_property_count(pRenameListIni, rlFilesSection); i++)
	{
		const char* name = ini_property_name(pRenameListIni, rlFilesSection, i);
		const char* value = ini_property_value(pRenameListIni, rlFilesSection, i);
		ini_property_add(pManifestIni, manFilesSection, name, (int)strlen(name), value, (int)strlen(value));
	}
	return 0;
}

int BuildResourceManifest(const char* resourceDirectory, const char* renameListPath, const char* manifestPath)
{
	asDebugLog("--------------------------------------------------------------------------------------------------------");
	asDebugLog("Building Manifest...");

	ini_t* pManifestIni = ini_create(NULL);
	int filesSection = ini_section_add(pManifestIni, "files", 6);

	/*Setup INI*/
	cf_traverse(resourceDirectory, ResourceFileAdd, &(ResourceAddInfo)
	{
		.filesSection = filesSection,
		.resourceDirectory = resourceDirectory,
		.pManifestIni = pManifestIni
	});

	/*Merge the Rename List*/
	ini_t* pRenameListIni = asCfgGetMattiasPtr(asCfgLoad(renameListPath));
	if (pRenameListIni)
	{
		MergeRenameList(pManifestIni, pRenameListIni);
		ini_destroy(pRenameListIni);
	}

	/*Save the INI to Memory*/
	int fileSize = ini_save(pManifestIni, NULL, 0);
	char* data = malloc(fileSize);
	ASASSERT(data);
	memset(data, 0, fileSize);
	fileSize = ini_save(pManifestIni, data, fileSize);
	ini_destroy(pManifestIni);
	asDebugLog("%s", data);

	/*Save to Disk*/
	FILE* fp = fopen(manifestPath, "w");
	if (!fp)
	{
		return -1;
	}
	fwrite(data, 1, (size_t)fileSize-1, fp);
	fclose(fp);
	
	return 0;
}
