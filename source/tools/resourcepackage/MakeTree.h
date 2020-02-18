#pragma once

#include <stdbool.h>
#include <engine/common/asConfigFile.h>
#include <engine/common/asStackContainer.h>


/*The information about the current stack state*/
typedef struct {
	char currentDirectory[1024]; /*Current Relative Directory*/
	char currentMakeFileDirectory[1024]; /*Current Relative Directory*/

	asCfgFile_t* _pRcMakeFile;
	int _ruleStackPosition; /*Top position in the extension rule stack*/
} RP_Folder_t;

typedef enum {
	RP_SCOPE_GLOBAL,
	RP_SCOPE_CLASS,
	RP_SCOPE_EXTENSION,
	RP_SCOPE_FILE,
	RP_SCOPE_MAX = UINT32_MAX
} RP_ScopeType_t;

/*The current state of the file
Name of rule is stored externally as a hash lookup*/
typedef struct {
	RP_ScopeType_t scope; /*Context: GLOBAL, CLASS, EXTENSION, FILE*/
	asHash32_t scopeNameHash; /*File/Extension/Command Name*/
	asHash32_t propNameHash; /*Name of property*/
} RP_Rule_Key_t;

/*The current state of the file
Name of rule is stored externally as a hash lookup*/
typedef struct {
	RP_Rule_Key_t originalKey; /*Resolves most collisions*/
	int associatedStateStackIndex;
	int makeFileCfgSectionID;
	int makeFileCfgSectionPropID;
} RP_Rule_Value_t;

/*Global Make Tree Context*/
typedef struct {
	/*Folder Stack to Rewind the State*/
	AS_STACK(RP_Folder_t) folderStack;

	/*Stack of Rules*/
	AS_STACK(asHash32_t) ruleKeyHashStack;
	AS_STACK(RP_Rule_Value_t) ruleValueStack;

	char outputBaseDirectory[1024];
	const char* binaryDirectory;
} RP_Context_t;

/*Find the dominant property name
Rules come in last to first say: GLOBAL, CLASS, EXTENSION, FILE*/
const char* RP_Context_FindDominantProperty(RP_Context_t* context, 
	const char* propertyName,
	const char* path,
	const char* className,
	const char* defaultValue);

/*Traversal callback*/
typedef int(*RP_TraversalCallback_t)(const RP_Context_t* context, const char*, void*);

/*Walk through the directories*/
int RP_Context_Traverse(
	const char* baseDirectory,
	const char* binDirectory,
	RP_Context_t* context,
	RP_TraversalCallback_t fpCallback,
	void* pUserData);