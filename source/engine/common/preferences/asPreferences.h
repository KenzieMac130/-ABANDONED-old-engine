#ifndef _ASPREFERENCES_H_
#define _ASPREFERENCES_H_

#include "../asCommon.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct asPreferenceManager asPreferenceManager;

/*
* Global Preferences
*/
ASEXPORT asPreferenceManager* asGetGlobalPrefs();
ASEXPORT void _asSetGlobalPrefs(asPreferenceManager* prefs);

typedef enum {
	AS_PREFTYPE_FLOAT,
	AS_PREFTYPE_INT32,
	AS_PREFTYPE_CSTRING,
	AS_PREFTYPE_NULL,
	AS_PREFTYPE_COUNT,
	AS_PREFTYPE_MAX = UINT32_MAX
} asPrefType;

typedef asResults(*asPrefChangeCallback)(const char* propName, void* pCurrentValue, void* pNewValueTmp, void* pUserData);

ASEXPORT asResults asPreferencesRegisterOpenSection(asPreferenceManager* pManager, const char* sectionName);
ASEXPORT asResults asPreferencesRegisterParamFloat(asPreferenceManager* pManager,  const char* name, float* pVal, float min, float max, bool saveLoad, asPrefChangeCallback changeCb, void* userData, const char* helpStr);
ASEXPORT asResults asPreferencesRegisterParamInt32(asPreferenceManager* pManager, const char* name, int32_t* pVal, int32_t min, int32_t max, bool saveLoad, asPrefChangeCallback changeCb, void* userData, const char* helpStr);
ASEXPORT asResults asPreferencesRegisterParamCString(asPreferenceManager* pManager, const char* name, char* pVal, size_t max, bool saveLoad, asPrefChangeCallback changeCb, void* userData, const char* helpStr);
ASEXPORT asResults asPreferencesRegisterNullFunction(asPreferenceManager* pManager, const char* name, asPrefChangeCallback changeCb, void* userData, const char* helpStr);

ASEXPORT asResults asPreferenceManagerCreate(asPreferenceManager** ppManager);
ASEXPORT asResults asPreferenceManagerDestroy(asPreferenceManager* pManager);

ASEXPORT asResults asPreferencesSetParam(asPreferenceManager* pManager, const char* section, const char* name, const char* valueStr);

ASEXPORT asResults asPreferencesPrintParamHelp(asPreferenceManager* pManager, const char* section, const char* name);
ASEXPORT asResults asPreferencesPrintParamValue(asPreferenceManager* pManager, const char* section, const char* name);

ASEXPORT asResults asPreferencesLoadIni(asPreferenceManager* pManager, const char* fileName);
ASEXPORT asResults asPreferencesLoadSection(asPreferenceManager* pManager, const char* section);
ASEXPORT asResults asPreferencesReleaseIni(asPreferenceManager* pManager);

ASEXPORT asResults asPreferencesSaveSectionsToIni(asPreferenceManager* pManager, const char* fileName);

/*Fetch Section Names*/
ASEXPORT size_t asPreferencesInspectGetSectionCount(asPreferenceManager* pManager);
ASEXPORT asResults asPreferencesInspectGetSectionNameTmp(asPreferenceManager* pManager, size_t idx, const char** ppName);
/*Fetch Entry Names*/
ASEXPORT size_t asPreferencesInspectGetEntryCount(asPreferenceManager* pManager, size_t sectionIdx);
ASEXPORT asResults asPreferencesInspectGetEntryNameTmp(asPreferenceManager* pManager, size_t sectionIdx, size_t idx, const char** ppName);

#ifdef __cplusplus
}
#endif
#endif