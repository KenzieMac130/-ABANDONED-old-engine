#include "MakeTree.h"

#include <engine/common/asStackContainer.h>
#include <engine/common/asCommon.h>

RP_Rule_Value_t _findLastPropertyWithKey(RP_Context_t* context, RP_Rule_Key_t key, RP_Rule_Value_t defaultVal)
{
	RP_Rule_Value_t result = defaultVal;
	asHash32_t keyHash = asHashBytes32_xxHash(&key, sizeof(key));
	const int stackSize = AS_STACK_LENGTH(context->ruleKeyHashStack);
	/*Forward search through stack*/
	for (int i = 0; i < stackSize; i++)
	{
		/*Check for key match*/
		RP_Rule_Key_t currentKey = AS_STACK_AT_INDEX(context->ruleValueStack, i).originalKey;
		if (memcmp(&currentKey, &key, sizeof(key)) == 0)
		{
			/*Newest Result, Keep Searching*/
			result = AS_STACK_AT_INDEX(context->ruleValueStack, i);
		}
	}
	return result;
}

const char* RP_Context_FindDominantProperty(RP_Context_t* context, const char* propertyName, const char* path, const char* className, const char* defaultValue)
{
	RP_Rule_Value_t value;
	value.associatedStateStackIndex = -1;

	/*Global Scope*/
	RP_Rule_Key_t key = (RP_Rule_Key_t){
						.scope = RP_SCOPE_GLOBAL,
						.scopeNameHash = 0,
						.propNameHash = asHashBytes32_xxHash(propertyName, strlen(propertyName)),
	};
	value = _findLastPropertyWithKey(context, key, value);

	/*Class Scope*/
	key.scope = RP_SCOPE_CLASS;
	key.scopeNameHash = asHashBytes32_xxHash(className, strlen(className));
	value = _findLastPropertyWithKey(context, key, value);

	/*Extension Scope*/
	const char* extName = strrchr(path, '.');
	if (extName)
	{
		key.scope = RP_SCOPE_EXTENSION;
		key.scopeNameHash = asHashBytes32_xxHash(extName, strlen(extName));
		value = _findLastPropertyWithKey(context, key, value);
	}

	/*File Name Scope*/
	const char* fileName = strrchr(path, '/');
	if (fileName)
	{
		key.scope = RP_SCOPE_FILE;
		key.scopeNameHash = asHashBytes32_xxHash(fileName, strlen(fileName));
		value = _findLastPropertyWithKey(context, key, value);
	}
	
	/*Return Default if Invalid*/
	if (value.associatedStateStackIndex < 0)
	{
		return defaultValue;
	}

	/*Fetch and return if Valid*/
	RP_Folder_t folder = AS_STACK_AT_INDEX(context->folderStack, value.associatedStateStackIndex);
	const char* finalName;
	const char* finalValue;
	asCfgGetPropertyAtSectionIndex(folder._pRcMakeFile, value.makeFileCfgSectionID, value.makeFileCfgSectionPropID, &finalName, &finalValue);
	return finalValue;
}

void _folderDestructor(void* ptr)
{
	RP_Folder_t* pFolder = (RP_Folder_t *)ptr;
	asCfgFree(pFolder->_pRcMakeFile);
}

int _addFolder(const char* nextDirectory, RP_Context_t* context)
{
	/*Add Folder to Stack*/
	AS_STACK_PUSH(context->folderStack);
	RP_Folder_t* pCurrentFolder = &AS_STACK_TOP_ENTRY(context->folderStack);
	pCurrentFolder->_ruleStackPosition = AS_STACK_LENGTH(context->ruleValueStack);
	memcpy(pCurrentFolder->currentDirectory, nextDirectory, 1024);
	asDebugLog("---> Working: %s", pCurrentFolder->currentDirectory);

	/*Open Make File*/
	char mkFilePath[1024];
	memset(mkFilePath, 0, 1024);
#if defined(WIN32)
	snprintf(mkFilePath, 1024, "%s\\AsResourceMake.txt", pCurrentFolder->currentDirectory);
#else
	snprintf(mkFilePath, 1024, "%s/AsResourceMake.txt", pCurrentFolder->currentDirectory);
#endif
	pCurrentFolder->_pRcMakeFile = asCfgLoad(mkFilePath);
	strcpy(pCurrentFolder->currentMakeFileDirectory, mkFilePath);

	if (!pCurrentFolder->_pRcMakeFile)
	{
		asDebugWarning("MAKE FILE NOT FOUND... Results May Be Mangled!");
		AS_STACK_POP(context->folderStack);
		exit(-2);
		return -1;
	}
	return 0;
}

void _closeFolder(RP_Context_t* context)
{
	RP_Folder_t* pCurrentFolder = &AS_STACK_TOP_ENTRY(context->folderStack);
	AS_STACK_DOWNSIZE_TO(context->ruleKeyHashStack, pCurrentFolder->_ruleStackPosition);
	AS_STACK_DOWNSIZE_TO(context->ruleValueStack, pCurrentFolder->_ruleStackPosition);
	asDebugLog("<--- Backed Out/Erased Rules From: %s", pCurrentFolder->currentDirectory);
	AS_STACK_POP(context->folderStack);
}

int RP_Context_Traverse(const char* baseDirectory, const char* binDirectory, RP_Context_t* context, RP_TraversalCallback_t fpCallback, void* pUserData)
{
	/*Set Binary Directory*/
	context->binaryDirectory = binDirectory;

	/*Initialize Stacks*/
	AS_STACK_INIT_WITH_DESTRUCTOR(context->folderStack, _folderDestructor, 32);
	AS_STACK_INIT(context->ruleKeyHashStack, 1024);
	AS_STACK_INIT(context->ruleValueStack, 1024);

	/*Open Base Folder*/
	const char* nextDirectory = baseDirectory;
	int levelsDeep = 1;
	if (_addFolder(nextDirectory, context)) 
	{
		return-1; 
	}

	/*Process Rules in File*/
	while (levelsDeep > 0)
	{
		bool EofReached = true;
		RP_Folder_t* pCurrentFolder = &AS_STACK_TOP_ENTRY(context->folderStack);

		/*For Each Section*/
		const char* sectionName = asCfgGetCurrentSectionName(pCurrentFolder->_pRcMakeFile);
		while (sectionName)
		{
			/*If Folder: Add Folder*/
			if (strcmp(sectionName, "FOLDER") == 0)
			{
				/*Get Folder Name*/
				const char* folderName = asCfgGetString(pCurrentFolder->_pRcMakeFile, "SUBDIRECTORY", "$ERROR$");

				/*Create New File Path*/
				char newFilePath[1024];
				memset(newFilePath, 0, 1024);
#if defined(WIN32)
				snprintf(newFilePath, 1024, "%s\\%s", pCurrentFolder->currentDirectory, folderName);
#else
				snprintf(newFilePath, 1024, "%s/%s", pCurrentFolder->currentDirectory, folderName);
#endif

				/*Open Folder*/
				if (_addFolder(newFilePath, context) == 0)
				{
					levelsDeep++;
					asCfgOpenNextSection(pCurrentFolder->_pRcMakeFile);
					EofReached = false;
					break;
				}
			}
			/*If Ruleset: Add Rules*/
			else if (strcmp(sectionName, "RULES") == 0)
			{
				/*Get Ruleset Context*/
				const char* contextName = asCfgGetString(pCurrentFolder->_pRcMakeFile, "RULES_CONTEXT", "GLOBAL");
				/*Get Ruleset Name*/
				const char* rulsetName = rulsetName = asCfgGetString(pCurrentFolder->_pRcMakeFile, "RULES_APPLY_NAME", "");

				asDebugLog("[RULESET] %s: %s", contextName, rulsetName);
				
				/*Name/Scope*/
				asHash32_t rulesetNameHash = asHashBytes32_xxHash(rulsetName, strlen(rulsetName));
				RP_ScopeType_t scope;
				if (strcmp(contextName, "GLOBAL") == 0) /*Global may not have ruleset name*/
				{
					scope = RP_SCOPE_GLOBAL;
					rulesetNameHash = 0;
				}
				else if (strcmp(contextName, "CLASS") == 0)
				{
					scope = RP_SCOPE_CLASS;
				}
				else if (strcmp(contextName, "EXTENSION") == 0)
				{
					scope = RP_SCOPE_EXTENSION;
				}
				else if (strcmp(contextName, "FILE") == 0)
				{
					scope = RP_SCOPE_FILE;
				}
				else
				{
					asDebugWarning("UNRECOGNIZED SCOPE: %s", contextName);
					continue;
				}

				/*Add All Rules*/
				const char* propName;
				const char* propValue;
				while (asCfgGetNextProp(pCurrentFolder->_pRcMakeFile, &propName, &propValue) == 0)
				{
					asDebugLog("[Prop] %s: %s", propName, propValue);

					AS_STACK_PUSH(context->ruleKeyHashStack);
					AS_STACK_PUSH(context->ruleValueStack);

					RP_Rule_Key_t key = (RP_Rule_Key_t){
						.scope = scope,
						.scopeNameHash = rulesetNameHash,
						.propNameHash = asHashBytes32_xxHash(propName, strlen(propName)),
					};

					AS_STACK_TOP_ENTRY(context->ruleKeyHashStack) = asHashBytes32_xxHash(&key, sizeof(key));

					AS_STACK_TOP_ENTRY(context->ruleValueStack) = (RP_Rule_Value_t){
						.originalKey = key,
						.associatedStateStackIndex = AS_STACK_TOP_INDEX(context->folderStack),
						.makeFileCfgSectionID = asCfgGetSectionIndex(pCurrentFolder->_pRcMakeFile),
						.makeFileCfgSectionPropID = asCfgGetPropIndex(pCurrentFolder->_pRcMakeFile)
					};
				}
			}

			/*Progress to Next Section*/
			asCfgOpenNextSection(pCurrentFolder->_pRcMakeFile);
			sectionName = asCfgGetCurrentSectionName(pCurrentFolder->_pRcMakeFile);
		}
		if (EofReached)
		{
			/*Process All Gathered Properties*/
			fpCallback(context, pCurrentFolder->currentDirectory, pUserData);

			/*Close Folder*/
			_closeFolder(context);
			levelsDeep--;
		}
	}

	/*Destroy Stacks*/
	AS_STACK_FREE(context->ruleValueStack);
	AS_STACK_FREE(context->ruleKeyHashStack);
	AS_STACK_FREE(context->folderStack);

	return 0;
}