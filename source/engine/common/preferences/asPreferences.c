#include "asPreferences.h"

#include "engine/common/asConfigFile.h"
#include "../thirdparty/mattias/ini.h"

#include "engine/resource/asUserFiles.h"

struct prefEntry {
	const char* name;
	asPrefType type;
	bool saveLoad;
	const char* helpStr;
	union {
		void* pValue;
		float* pFValue;
		int32_t* pI32Value;
		char* pStrValue;
	};
	union {
		struct {
			float fMin;
			float fMax;
		};
		struct {
			int32_t i32Min;
			int32_t i32Max;
		};
		struct {
			size_t strMax;
		};
	};
	asPrefChangeCallback changeCallback;
	void* userData;
};

struct prefSection {
	bool valid; /*Work around for STB_DS bug...*/
	struct { char* key; struct prefEntry value; }*prefHM;
};

struct asPreferenceManager {
	int64_t activeSectionId;
	struct { char* key; struct prefSection value; }*sectionHM;
	ini_t* pIniFile;
};

asPreferenceManager* globalPrefs;
ASEXPORT asPreferenceManager* asGetGlobalPrefs() { return globalPrefs; }
ASEXPORT void _asSetGlobalPrefs(asPreferenceManager* prefs)
{
	globalPrefs = prefs;
}

/*Create/Destroy*/
ASEXPORT asResults asPreferenceManagerCreate(asPreferenceManager** ppManager)
{
	*ppManager = asMalloc(sizeof(asPreferenceManager));
	memset(*ppManager, 0, sizeof(asPreferenceManager));
	asPreferenceManager* prefMan = *ppManager;
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferenceManagerDestroy(asPreferenceManager* pManager)
{
	for (int i = 0; i < shlen(pManager->sectionHM); i++)
	{
		shfree(pManager->sectionHM[i].value.prefHM);
	}
	shfree(pManager->sectionHM);
	if (pManager->pIniFile) { ini_destroy(pManager->pIniFile); }
	asFree(pManager);
	return AS_SUCCESS;
}

/*Registration*/
ASEXPORT asResults asPreferencesRegisterOpenSection(asPreferenceManager* pManager, const char* sectionName)
{
	struct prefSection section = { true };
	shput(pManager->sectionHM, sectionName, section);
	pManager->activeSectionId = shgeti(pManager->sectionHM, sectionName);
	return AS_SUCCESS;
}

#define _PUTPARAM(_manager, _str, _entry) shput(_manager->sectionHM[_manager->activeSectionId].value.prefHM, _str, _entry)

ASEXPORT asResults asPreferencesRegisterParamFloat(asPreferenceManager* pManager, const char* name, float* pVal, float min, float max, bool saveLoad, asPrefChangeCallback changeCb, void* userData, const char* helpStr)
{
	struct prefEntry entry = (struct prefEntry){ 0 };
	entry.name = name;
	entry.saveLoad = saveLoad;
	entry.helpStr = helpStr;
	entry.userData = userData;
	entry.changeCallback = changeCb;

	entry.type = AS_PREFTYPE_FLOAT;
	entry.pFValue = pVal;
	entry.fMin = min;
	entry.fMax = max;

	_PUTPARAM(pManager, name, entry);
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferencesRegisterParamInt32(asPreferenceManager* pManager, const char* name, int32_t* pVal, int32_t min, int32_t max, bool saveLoad, asPrefChangeCallback changeCb, void* userData, const char* helpStr)
{
	struct prefEntry entry = (struct prefEntry){ 0 };
	entry.name = name;
	entry.saveLoad = saveLoad;
	entry.helpStr = helpStr;
	entry.userData = userData;
	entry.changeCallback = changeCb;

	entry.type = AS_PREFTYPE_INT32;
	entry.pI32Value = pVal;
	entry.i32Min = min;
	entry.i32Max = max;

	_PUTPARAM(pManager, name, entry);
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferencesRegisterParamCString(asPreferenceManager* pManager, const char* name, char* pVal, size_t max, bool saveLoad, asPrefChangeCallback changeCb, void* userData, const char* helpStr)
{
	struct prefEntry entry = (struct prefEntry){ 0 };
	entry.name = name;
	entry.saveLoad = saveLoad;
	entry.helpStr = helpStr;
	entry.userData = userData;
	entry.changeCallback = changeCb;

	entry.type = AS_PREFTYPE_CSTRING;
	entry.pStrValue = pVal;
	entry.strMax = max;

	_PUTPARAM(pManager, name, entry);
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferencesRegisterNullFunction(asPreferenceManager* pManager, const char* name, asPrefChangeCallback changeCb, void* userData, const char* helpStr)
{
	struct prefEntry entry = (struct prefEntry){ 0 };
	entry.name = name;
	entry.saveLoad = false;
	entry.helpStr = helpStr;
	entry.userData = userData;
	entry.changeCallback = changeCb;

	entry.type = AS_PREFTYPE_NULL;

	_PUTPARAM(pManager, name, entry);
	return AS_SUCCESS;
}

asResults _preferencesSetParam(asPreferenceManager* pManager, const char* sectionName, const char* name, const char* valueStrEntered, bool fromIni)
{
	/*Find Section*/
	struct prefSection* pSection = &shget(pManager->sectionHM, sectionName);
	if (!pSection->valid) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }

	/*Find Param*/
	struct prefEntry* pEntry = &shget(pSection->prefHM, name);
	if (!pEntry->name) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }

	/*Don't Load from Ini if asked not to*/
	if (!pEntry->saveLoad && fromIni) { return AS_SUCCESS; }

	/*No Value might be Acceptable*/
	if (!valueStrEntered && pEntry->type != AS_PREFTYPE_NULL) { return AS_FAILURE_INVALID_PARAM; }
	const char* valueStr = valueStrEntered;
	if (!valueStr) { valueStr = "NULL"; }

	/*Convert String to Number*/
	double valueDouble = atof(valueStr);

	/*Map Data and Solve Constraints*/
	size_t valueSize = 0;
	void* pNewValue = NULL;
	float NewFloat = (float)valueDouble;
	int32_t NewInt32 = (int32_t)valueDouble;
	switch (pEntry->type)
	{
	case AS_PREFTYPE_FLOAT:
		valueSize = sizeof(float);
		if (NewFloat > pEntry->fMax) { NewFloat = pEntry->fMax; }
		if (NewFloat < pEntry->fMin) { NewFloat = pEntry->fMin; }
		pNewValue = &NewFloat;
		break;
	case AS_PREFTYPE_INT32:
		valueSize = sizeof(int32_t);
		if (NewInt32 > pEntry->i32Max) { NewInt32 = pEntry->i32Max; }
		if (NewInt32 < pEntry->i32Min) { NewInt32 = pEntry->i32Min; }
		pNewValue = &NewInt32;
		break;
	case AS_PREFTYPE_CSTRING:
		valueSize = strlen(valueStr);
		if (valueSize > pEntry->strMax) { valueSize = pEntry->strMax; }
		pNewValue = (void*)valueStr;
		break;
	default:
		break;
	}

	/*Apply Change*/
	if (pEntry->changeCallback)
	{
		return pEntry->changeCallback(pEntry->name, pEntry->pValue, pNewValue, pEntry->userData);
	}
	else
	{
		if (!pNewValue || !valueSize) { return AS_SUCCESS; }
		memcpy(pEntry->pValue, pNewValue, valueSize);
		return AS_SUCCESS;
	}
}

/*Setting/Calling*/
ASEXPORT asResults asPreferencesSetParam(asPreferenceManager* pManager, const char* sectionName, const char* name, const char* valueStrEntered)
{
	return _preferencesSetParam(pManager, sectionName, name, valueStrEntered, false);
}

ASEXPORT asResults asPreferencesPrintParamHelp(asPreferenceManager* pManager, const char* sectionName, const char* name)
{
	/*Find Section*/
	struct prefSection* pSection = &shget(pManager->sectionHM, sectionName);
	if (!pSection->valid) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }

	/*Find Param*/
	struct prefEntry* pEntry = &shget(pSection->prefHM, name);
	if (!pEntry->name) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }

	/*If No Help Available*/
	if (!pEntry->helpStr) { 
		asDebugLog("No Help Available for \"%s.%s\"... :(", sectionName, name); 
		return AS_SUCCESS; 
	}

	asDebugLog("\"%s.%s\"?: %s", sectionName, name, pEntry->helpStr);
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferencesPrintParamValue(asPreferenceManager* pManager, const char* sectionName, const char* name)
{
	/*Find Section*/
	struct prefSection* pSection = &shget(pManager->sectionHM, sectionName);
	if (!pSection->valid) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }

	/*Find Param*/
	struct prefEntry* pEntry = &shget(pSection->prefHM, name);
	if (!pEntry->name) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }

	if(!pEntry->pValue) { 
		asDebugLog("Direct Access to Value Not Available for \"%s.%s\"... :(", sectionName, name);
		return AS_SUCCESS;
	}

	switch (pEntry->type)
	{
	case AS_PREFTYPE_FLOAT:
		asDebugLog("\"%s.%s\" = %f", sectionName, name, *pEntry->pFValue);
		break;
	case AS_PREFTYPE_INT32:
		asDebugLog("\"%s.%s\" = %d", sectionName, name, *pEntry->pI32Value);
		break;
	case AS_PREFTYPE_CSTRING:
		asDebugLog("\"%s.%s\" = %.*s", sectionName, name, pEntry->strMax, pEntry->pStrValue);
		break;
	default:
		asDebugLog("Command \"%s.%s\" is a Pure Function", sectionName, name);
		break;
	}

	return AS_SUCCESS;
}

/*Load/Save*/
ASEXPORT asResults asPreferencesLoadIni(asPreferenceManager* pManager, const char* fileName)
{
	asCfgFile_t* pConfig = asCfgLoadUserFile(fileName);
	pManager->pIniFile = asCfgGetMattiasPtr(pConfig);
	asCfgFree_KeepMattias(pConfig);
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferencesLoadSection(asPreferenceManager* pManager, const char* sectionName)
{
	if (!pManager->pIniFile) { return AS_SUCCESS; } /*It's normal to fail load if ini never existed*/
	int iniSectionId = ini_find_section(pManager->pIniFile, sectionName, strlen(sectionName));
	if (iniSectionId == INI_NOT_FOUND) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }
	for (int i = 0; i < ini_property_count(pManager->pIniFile, iniSectionId); i++)
	{
		const char* propName = ini_property_name(pManager->pIniFile, iniSectionId, i);
		const char* propValue = ini_property_value(pManager->pIniFile, iniSectionId, i);
		_preferencesSetParam(pManager, sectionName, propName, propValue, true);
	}
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferencesReleaseIni(asPreferenceManager* pManager)
{
	if (pManager->pIniFile) { ini_destroy(pManager->pIniFile); }
	return AS_SUCCESS;
}

ASEXPORT asResults asPreferencesSaveSectionsToIni(asPreferenceManager* pManager, const char* fileName)
{
	ini_t* saverIni = ini_create(NULL);
	
	for (int i = 0; i < shlen(pManager->sectionHM); i++)
	{
		const struct prefSection section = pManager->sectionHM[i].value;
		const char* sectionName = pManager->sectionHM[i].key;
		int iniSectionId = INI_NOT_FOUND;
		for (int j = 0; j < shlen(section.prefHM); j++)
		{
			const char* entryName = section.prefHM[j].key;
			const struct prefEntry entry = section.prefHM[j].value;
			if (!entry.saveLoad) { continue; }

			if(iniSectionId == INI_NOT_FOUND)
				iniSectionId = ini_section_add(saverIni, sectionName, strlen(sectionName));

			char valueName[128];
			memset(valueName, 0, 128);
			const char* dataPtr = valueName;
			size_t dataStrLen = 0;
			switch (entry.type)
			{
			case AS_PREFTYPE_FLOAT:
				snprintf(valueName, 127, "%f", *entry.pFValue);
				dataStrLen = strlen(dataPtr);
				break;
			case AS_PREFTYPE_INT32:
				snprintf(valueName, 127, "%d", *entry.pI32Value);
				dataStrLen = strlen(dataPtr);
				break;
			case AS_PREFTYPE_CSTRING:
				dataPtr = entry.pStrValue;
				dataStrLen = strnlen(dataPtr, entry.strMax);
				break;
			case AS_PREFTYPE_NULL:
				continue;
				break;
			}
			ini_property_add(saverIni, iniSectionId, entryName, strlen(entryName), dataPtr, dataStrLen);
		}
	}

	asUserFile userFile;
	if (asUserFileOpen(&userFile, fileName, strlen(fileName), "w") != AS_SUCCESS)
		return AS_FAILURE_FILE_INACCESSIBLE;
	size_t fileSize = ini_save(saverIni, NULL, 0);
	char* fileData = asMalloc(fileSize);
	memset(fileData, 0, fileSize);
	ini_save(saverIni, fileData, fileSize);
	asUserFileWrite(fileData, 1, fileSize-1, &userFile);
	asUserFileClose(&userFile);
	return AS_SUCCESS;
}

ASEXPORT size_t asPreferencesInspectGetSectionCount(asPreferenceManager* pManager)
{
	return shlen(pManager->sectionHM);
}

ASEXPORT asResults asPreferencesInspectGetSectionNameTmp(asPreferenceManager* pManager, size_t idx, const char** ppName)
{
	if (idx > shlen(pManager->sectionHM)) { return AS_FAILURE_OUT_OF_BOUNDS; }
	*ppName = pManager->sectionHM[idx].key;
	return AS_SUCCESS;
}

ASEXPORT size_t asPreferencesInspectGetEntryCount(asPreferenceManager* pManager, size_t sectionIdx)
{
	if (sectionIdx > shlen(pManager->sectionHM)) { return 0; }
	return shlen(pManager->sectionHM[sectionIdx].value.prefHM);
}

ASEXPORT asResults asPreferencesInspectGetEntryNameTmp(asPreferenceManager* pManager, size_t sectionIdx, size_t idx, const char** ppName)
{
	if (sectionIdx > shlen(pManager->sectionHM)) { return AS_FAILURE_OUT_OF_BOUNDS; }
	if (idx > shlen(pManager->sectionHM[sectionIdx].value.prefHM)) { return AS_FAILURE_OUT_OF_BOUNDS; }
	*ppName = pManager->sectionHM[sectionIdx].value.prefHM[idx].key;
	return AS_SUCCESS;
}