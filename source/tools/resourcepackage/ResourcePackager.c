#include <stdio.h>

#include "MakeTree.h"
#include "ManifestBuild.h"

#define CUTE_FILES_IMPLEMENTATION
#include <cute/cute_files.h>

#include <engine/resource/asResource.h>

void FormatCommand(char* outBuffer, size_t bufferSize, const char* inFormat, const char* inPath, const char* outPath, const char* fileName, const char* extraSettings, const char* binDir)
{
	memset(outBuffer, 0, bufferSize);
	size_t buff_Idx = 0;
	size_t format_Idx = 0;
	while (buff_Idx < bufferSize && format_Idx < strlen(inFormat))
	{
		/*OS Path Slash Format*/
		if (!strncmp(&inFormat[format_Idx], "@%/", 3))
		{
#if defined(WIN32)
			strncat(&outBuffer[buff_Idx], "\\", 1024);
#else
			strncat(&outBuffer[buff_Idx], "/", 1024);
#endif
			buff_Idx += 1;
			format_Idx += 3;
		}
		/*Input Format*/
		else if (!strncmp(&inFormat[format_Idx], "@%I", 3))
		{
			strncat(&outBuffer[buff_Idx], inPath, 1024);
			buff_Idx += strlen(inPath);
			format_Idx += 3;
		}
		/*Output Format*/
		else if (!strncmp(&inFormat[format_Idx], "@%O", 3))
		{
			strncat(&outBuffer[buff_Idx], outPath, 1024);
			buff_Idx += strlen(outPath);
			format_Idx += 3;
		}
		/*File Name Format*/
		else if (!strncmp(&inFormat[format_Idx], "@%N", 3))
		{
			strncat(&outBuffer[buff_Idx], fileName, 1024);
			buff_Idx += strlen(fileName);
			format_Idx += 3;
		}
		/*Settings Format*/
		else if (!strncmp(&inFormat[format_Idx], "@%S", 3))
		{
			strncat(&outBuffer[buff_Idx], extraSettings, 1024);
			buff_Idx += strlen(extraSettings);
			format_Idx += 3;
		}
		/*Bin Directory Format*/
		else if (!strncmp(&inFormat[format_Idx], "@%B", 3))
		{
			strncat(&outBuffer[buff_Idx], binDir, 1024);
			buff_Idx += strlen(binDir);
			format_Idx += 3;
		}

		outBuffer[buff_Idx] = inFormat[format_Idx];
		format_Idx++;
		buff_Idx++;
	}
}

typedef struct {
	AS_STACK(asHash32_t) raisedDepenencies; /*Lised of altered a pre-requisite in process*/
} DependencyBuffer;

void InitDepencencyBuffer(DependencyBuffer* pDepBuffer)
{
	memset(pDepBuffer, 0, sizeof(pDepBuffer));
	AS_STACK_INIT(pDepBuffer->raisedDepenencies, 64);
}

void ShutdownDependencyBuffer(DependencyBuffer* pDepBuffer)
{
	AS_STACK_FREE(pDepBuffer->raisedDepenencies);
}

/*Mark Dependency as Altered*/
void RaiseDepenency(DependencyBuffer* pDepBuffer, const char* name)
{
	AS_STACK_PUSH(pDepBuffer->raisedDepenencies);
	AS_STACK_TOP_ENTRY(pDepBuffer->raisedDepenencies) = asHashBytes32_xxHash(name, strlen(name));
}

bool isDependencyChanged(DependencyBuffer* pDepBuffer, const char* name)
{
	asHash32_t nameHash = asHashBytes32_xxHash(name, strlen(name));
	for (int i = 0; i < AS_STACK_LENGTH(pDepBuffer->raisedDepenencies); i++)
	{
		if (AS_STACK_AT_INDEX(pDepBuffer->raisedDepenencies, i) == nameHash)
		{
			return true;
		}
	}
	return false;
}

struct FolderTraversalData {
	bool forceBuildAllGlobal;
	cf_time_t lastManifestWriteTime;
	DependencyBuffer* pDepBuffer;
};

struct FileTraversalData {
	const RP_Context_t* pContext;
	bool forceBuildAllFolder;
	cf_time_t lastManifestWriteTime;
	DependencyBuffer* pDepBuffer;
};

void FileProcess(cf_file_t* pFile, void* pData)
{
	struct FileTraversalData* pFileTraversalData = (struct FileTraversalData*)pData;
	const RP_Context_t* pContext = pFileTraversalData->pContext;

	/*Exclude Make File*/
	if (!strcmp(pFile->name, "AsResourceMake.txt"))
	{
		return;
	}

	/*Get Class Name*/
	const char* className = RP_Context_FindDominantProperty(pContext, "RULES_CLASS", pFile->path, "", "");

	char outPath[1024];
	memset(outPath, 0, 1024);
	strncat(outPath, pContext->outputBaseDirectory, 1023);
	strncat(outPath, RP_Context_FindDominantProperty(pContext, "outDirectory", pFile->path, className, ""), 1023 - strlen(outPath));
#if defined(WIN32)
	/*Fix Out Path for Windows*/
	for (int i = 0; i < 1023; i++)
	{
		if(outPath[i] == '/')
		{
			outPath[i] = '\\';
		}
	}
	/*Fix Input Path for Windows*/
	for (int i = 0; i < strlen(pFile->path); i++)
	{
		if (pFile->path[i] == '/')
		{
			pFile->path[i] = '\\';
		}
	}
#endif

	/*Create Out Folder if it Doesn't Exist*/
	if (!cf_file_exists(outPath))
	{
		char createDirBuffer[2048];
		memset(createDirBuffer, 0, 2048);
		snprintf(createDirBuffer, 2048, "mkdir \"%s\"", outPath);
		system(createDirBuffer);
		asDebugLog("[CREATED MISSING DIRECTORY!] %s", outPath);
	}

	/*Get Command*/
	const char* commandFormat = RP_Context_FindDominantProperty(pContext, "OS_universalCommand", pFile->path, className, "");

#if defined(WIN32)
	commandFormat = RP_Context_FindDominantProperty(pContext, "OS_WindowsCommand", pFile->path, className, commandFormat);
#elif defined(__linux__)
	commandFormat = RP_Context_FindDominantProperty(pContext, "OS_LinuxCommand", pFile->path, className, commandFormat);
#elif defined(__APPLE__)
	commandFormat = RP_Context_FindDominantProperty(pContext, "OS_MacOSCommand", pFile->path, className, commandFormat);
#endif

	/*Find Dependencies that Requires Rebuild*/
	bool updateFromDependency = false;
	const char* dependencyName = RP_Context_FindDominantProperty(pContext, "depends", pFile->path, className, "");
	if (isDependencyChanged(pFileTraversalData->pDepBuffer, dependencyName))
	{
		updateFromDependency = true;
	}

	/*Only Update if out of Date or Dependency Changed*/
	if (!pFileTraversalData->forceBuildAllFolder && !updateFromDependency)
	{
		/*If Up To Date, Don't Rebuild*/
		cf_time_t assetLastWrite;
		memset(&assetLastWrite, 0, sizeof(assetLastWrite));
		cf_get_file_time(pFile->path, &assetLastWrite);
		if (cf_compare_file_times(&pFileTraversalData->lastManifestWriteTime, &assetLastWrite) >= 0)
		{
			return;
		}
	}

	/*FileName Only*/
	char fileName[256];
	memset(fileName, 0, 256);
	strcat(fileName, pFile->name);
	char* pos = strrchr(fileName, '.');
	if (pos)
	{
		*pos = '\0';
	}

	/*Run Command*/
	char command[4096];
	FormatCommand(command, 4096,
		commandFormat,
		pFile->path,
		outPath,
		fileName,
		RP_Context_FindDominantProperty(pContext, "commandSettings", pFile->path, className, ""),
		pContext->binaryDirectory);
	if (strcmp(command, "__NOOP__") && strcmp(command, "")) /*No Command*/
	{
		asDebugLog("--------------------------------------------------------------------------------------------------------");
		asDebugLog("[COMMAND]> %s", command);
		int returnCode = system(command);
		if (returnCode)
		{
			asDebugLog("[ERROR]> BAD RETURN CODE: %d CONTINUE AND DEEM SUCCESS? (y/n)", returnCode);
			if (getchar() != 'y')
			{
				exit(-1);
			}
		}
		asDebugLog("--------------------------------------------------------------------------------------------------------");
	}

	/*Raise Dependency*/
	const char* depToRaise = RP_Context_FindDominantProperty(pContext, "raiseDependency", pFile->path, className, "");
	if (strcmp(depToRaise, ""))
	{
		RaiseDepenency(pFileTraversalData->pDepBuffer, depToRaise);
	}
}

int FolderProcess(const RP_Context_t* pContext, const char* path, void* pData)
{
	struct FolderTraversalData* pFolderTraversalData = (struct FolderTraversalData*)pData;
	bool forceBuildFolder = false;

	/*Exclude folder if No Make File*/
	const char* makeFilePath = AS_STACK_TOP_ENTRY(pContext->folderStack).currentMakeFileDirectory;
	if (!cf_file_exists(makeFilePath))
	{
		return -1;
	}
	
	/*If Global Rebuild Rebuild all In Folder*/
	forceBuildFolder = pFolderTraversalData->forceBuildAllGlobal;

	/*If Make File is Newer than Manifest Force Rebuild of Folder*/
	cf_time_t makeFileWriteTime;
	if (!cf_get_file_time(makeFilePath, &makeFileWriteTime))
	if (cf_compare_file_times(&makeFileWriteTime, &pFolderTraversalData->lastManifestWriteTime) >= 0)
	{
		forceBuildFolder = true;
	}
	
	/*Traverse Each File*/
	struct FileTraversalData traversalData = (struct FileTraversalData){
		.pContext = pContext,
		.lastManifestWriteTime = pFolderTraversalData->lastManifestWriteTime,
		.forceBuildAllFolder = forceBuildFolder,
		.pDepBuffer = pFolderTraversalData->pDepBuffer
	};
	cf_traverse_norecursive(path, FileProcess, &traversalData);
	return 0;
}

int main(int argc, char* argv[])
{
	RP_Context_t rpCtx;
	memset(&rpCtx, 0, sizeof(rpCtx));

	char assetPath[1024];
	char resourcePath[1024];
	char defaultRaisedDependencies[1024]; /*Dependencies that might be required by a change in code format*/
	memset(assetPath, 0, 1024);
	memset(resourcePath, 0, 1024);
	memset(defaultRaisedDependencies, 0, 1024);
	bool forceRebuild = false;

	/*Config*/
	if (argc < 2) /*Asset Path*/
	{
		asDebugLog("Input (full) Working Asset Path:");
		fgets(&assetPath, 1024, stdin);
		for (int i = 0; i < 1024; i++) { if (assetPath[i] == '\n') { assetPath[i] = '\0'; } }
	}
	else
	{
		strncat(assetPath, argv[1], 1023);
		asDebugLog("%s", assetPath);
	}
	if (argc < 3) /*Resource Path*/
	{
		/*Generate Resource Path (assumes basic install)*/
		asDebugLog("Detected resource path: %s", asResource_GetResourceFolderPath());
		asDebugLog("Change? (type new) or Continue? (hit enter)");
		fgets(&resourcePath, 1024, stdin);
		for (int i = 0; i < 1024; i++) { if (resourcePath[i] == '\n') { resourcePath[i] = '\0'; } }
		if (!strcmp(resourcePath, "")) {
			strncat(resourcePath, asResource_GetResourceFolderPath(), 1023);
		}
	}
	else
	{
		strncat(resourcePath, argv[2], 1023);
		asDebugLog("%s", resourcePath);
	}
	if (argc < 4) /*Dependencies*/
	{
		/*Inject Additional Dependencies*/
		asDebugLog("Add Dependency Changes? (enter to skip):");
		fgets(&defaultRaisedDependencies, 1024, stdin);
		for (int i = 0; i < 1024; i++) { if (defaultRaisedDependencies[i] == '\n') { defaultRaisedDependencies[i] = '\0'; } }
	}
	else
	{
		strncat(defaultRaisedDependencies, argv[3], 1023);
		asDebugLog("%s", defaultRaisedDependencies);
	}
	if (argc < 5) /*Force Rebuild*/
	{
		/*Force Rebuild All*/
		asDebugLog("Force Rebuild? [WARNING, WILL TAKE A WHILE] (y/n)");
		if (getchar() == 'y')
		{
			forceRebuild = true;
		}
	}
	else
	{
		if (!strcmp(argv[4], "y"))
		{
			forceRebuild = true;
		}
	}

	/*Set Resource Folder*/
	strncat(rpCtx.outputBaseDirectory, resourcePath, 1023);
	asDebugLog("%s", rpCtx.outputBaseDirectory);

	/*Get Manifest Build Time*/
	char manifestPath[1024];
	memset(manifestPath, 0, 1024);
	snprintf(manifestPath, 1024, "%sresources.cfg", rpCtx.outputBaseDirectory);
	cf_time_t manifestBuildTime;
	memset(&manifestBuildTime, 0, sizeof(manifestBuildTime));
	cf_get_file_time(manifestPath, &manifestBuildTime);
	asDebugLog("%s", manifestPath);

	/*Init Dependency Buffer*/
	DependencyBuffer depBuffer;
	InitDepencencyBuffer(&depBuffer);

	/*Inject External Dependencies*/
	char* depStrTok = strtok(defaultRaisedDependencies, ",");
	while (depStrTok)
	{
		RaiseDepenency(&depBuffer, depStrTok);
		asDebugLog("EXTERNAL DEP: %s", depStrTok);
		depStrTok = strtok(NULL, ",");
	}

	/*Traverse*/
	struct FolderTraversalData traversalData = (struct FolderTraversalData){
		.forceBuildAllGlobal = forceRebuild,
		.lastManifestWriteTime = manifestBuildTime,
		.pDepBuffer = &depBuffer
	};
	RP_Context_Traverse(assetPath, asResource_GetBinFolderPath(), &rpCtx, &FolderProcess, &traversalData);

	/*Shutdown Dependency Buffer*/
	ShutdownDependencyBuffer(&depBuffer);

	/*Build Manifest*/
	char renameConfigPath[1024];
	memset(renameConfigPath, 0, 1024);
#if defined(WIN32)
	snprintf(renameConfigPath, 1024, "%s\\AsResourceRedirect.cfg", assetPath);
#else
	snprintf(renameConfigPath, 1024, "%s/AsResourceRedirect.cfg", assetPath);
#endif
	BuildResourceManifest(rpCtx.outputBaseDirectory, renameConfigPath, manifestPath);
	
	return 0;
};