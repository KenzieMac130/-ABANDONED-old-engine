#include "asConfigFile.h"
#include "../resource/asUserFiles.h"

/*Config file*/
#define INI_IMPLEMENTATION
#include <mattias/ini.h>

struct asCfgFile_t
{
	ini_t *pIni;
	int currentSection;
	int nextProperty;
};

ASEXPORT asCfgFile_t* asCfgLoad(const char* path)
{
	if (!path)
		return NULL;
	FILE *fp;
	fopen_s(&fp, path, "rb");
	if (!fp) {
		asDebugLog("Couldn't Open: %s", path);
		return NULL;
	}
	asDebugLog("Opened: %s", path);
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp) + 1;
	fseek(fp, 0, SEEK_SET);
	char* pCfgData = (char*)asMalloc(size);
	ASASSERT(pCfgData);
	fread(pCfgData, 1, size, fp);
	pCfgData[size - 1] = '\0';
	fclose(fp);
	asCfgFile_t *result = asMalloc(sizeof(asCfgFile_t));
	ASASSERT(result);
	result->pIni = ini_load(pCfgData, NULL);
	result->currentSection = INI_GLOBAL_SECTION;
	result->nextProperty = 0;
	asFree(pCfgData);
	return result;
}
ASEXPORT asCfgFile_t* asCfgLoadUserFile(const char* path)
{
	if (!path)
		return NULL;
	asUserFile userFile;
	if (asUserFileOpen(&userFile, path, strlen(path), "rb") != AS_SUCCESS) {
		asDebugLog("Couldn't Open: %s", path);
		return NULL;
	}
	asDebugLog("Opened: %s", path);

	int size = asUserFileGetSize(&userFile) + 1;
	char* pCfgData = (char*)asMalloc(size);
	ASASSERT(pCfgData);
	asUserFileRead(pCfgData, 1, size, &userFile);
	pCfgData[size - 1] = '\0';

	asCfgFile_t* result = asMalloc(sizeof(asCfgFile_t));
	ASASSERT(result);
	result->pIni = ini_load(pCfgData, NULL);
	result->currentSection = INI_GLOBAL_SECTION;
	result->nextProperty = 0;
	asUserFileClose(&userFile);
	return result;
}
ASEXPORT asCfgFile_t* asCfgFromMem(unsigned char* data, size_t size)
{
	asCfgFile_t *result = asMalloc(sizeof(asCfgFile_t));
	ASASSERT(result);
	result->pIni = ini_load(data, NULL);
	result->currentSection = INI_GLOBAL_SECTION;
	result->nextProperty = 0;
	return result;
}
ASEXPORT asCfgFile_t* asCfgCreateBlank()
{
	asCfgFile_t* result = asMalloc(sizeof(asCfgFile_t));
	ASASSERT(result);
	result->pIni = ini_create(NULL);
	result->currentSection = INI_GLOBAL_SECTION;
	result->nextProperty = 0;
	return result;
}
ASEXPORT void asCfgFree(asCfgFile_t* cfg)
{
	if (!cfg)
		return;
	ini_destroy(cfg->pIni);
	asFree(cfg);
}
ASEXPORT ini_t* asCfgGetMattiasPtr(asCfgFile_t* pCfg)
{
	if (!pCfg)
		return NULL;
	return pCfg->pIni;
}
ASEXPORT int asCfgOpenSectionByIndex(asCfgFile_t* pCfg, int index)
{
	if (!pCfg)
		return -1;
	if (index >= ini_section_count(pCfg->pIni))
		return -2;
	pCfg->currentSection = index;
	pCfg->nextProperty = 0;
	return 0;
}
ASEXPORT int asCfgOpenSection(asCfgFile_t* pCfg, const char* name)
{
	if (!pCfg)
		return -1;
	int section = ini_find_section(pCfg->pIni, name, (int)strlen(name));
	if (section == INI_NOT_FOUND)
		return -2;
	pCfg->currentSection = section;
	pCfg->nextProperty = 0;
	return 0;
}
ASEXPORT int asCfgOpenNextSection(asCfgFile_t* pCfg)
{
	if (pCfg->currentSection >= ini_section_count(pCfg->pIni))
		return -1;
	pCfg->currentSection++;
	return 0;
}
ASEXPORT const char* asCfgGetCurrentSectionName(asCfgFile_t* pCfg)
{
	if (pCfg->currentSection >= ini_section_count(pCfg->pIni) || pCfg->currentSection < 0)
		return NULL;
	return ini_section_name(pCfg->pIni, pCfg->currentSection);
}
ASEXPORT int asCfgGetSectionIndex(asCfgFile_t* pCfg)
{
	return pCfg->currentSection;
}
ASEXPORT int asCfgGetPropIndex(asCfgFile_t* pCfg)
{
	return pCfg->nextProperty -	1;
}
ASEXPORT double asCfgGetNumber(asCfgFile_t* pCfg, const char* name, double fallback)
{
	if (!pCfg)
		return fallback;
	int index = ini_find_property(pCfg->pIni, pCfg->currentSection, name, (int)strlen(name));
	if (index == INI_NOT_FOUND)
		return fallback;
	pCfg->nextProperty = index + 1;
	const char* propStr = ini_property_value(pCfg->pIni, pCfg->currentSection, index);
	char* endptr;
	double value = strtod(propStr, &endptr);
	if (endptr)
		return value;
	else
		return fallback;
}
ASEXPORT const char* asCfgGetString(asCfgFile_t* pCfg, const char* name, const char* fallback)
{
	if (!pCfg)
		return fallback;
	int index = ini_find_property(pCfg->pIni, pCfg->currentSection, name, (int)strlen(name));
	if (index == INI_NOT_FOUND)
		return fallback;
	pCfg->nextProperty = index + 1;
	const char* propStr = ini_property_value(pCfg->pIni, pCfg->currentSection, index);
	if (propStr)
		return propStr;
	else
		return fallback;
}

ASEXPORT int asCfgGetPropertyAtSectionIndex(asCfgFile_t* pCfg, int sectionIdx, int index, const char** ppName, const char** ppValue)
{
	if (!pCfg)
		return -1;
	if (sectionIdx >= ini_section_count(pCfg->pIni))
		return -2;
	if (index >= ini_property_count(pCfg->pIni, sectionIdx))
		return -3;
	*ppValue = ini_property_value(pCfg->pIni, sectionIdx, index);
	*ppName = ini_property_name(pCfg->pIni, sectionIdx, index);
	return 0;
}

ASEXPORT int asCfgGetNextProp(asCfgFile_t* pCfg, const char** ppName, const char** ppValue)
{
	if (!pCfg)
		return -1;
	if (pCfg->nextProperty >= ini_property_count(pCfg->pIni, pCfg->currentSection))
		return -2;
	*ppValue = ini_property_value(pCfg->pIni, pCfg->currentSection, pCfg->nextProperty);
	*ppName = ini_property_name(pCfg->pIni, pCfg->currentSection, pCfg->nextProperty);
	pCfg->nextProperty++;
	return 0;
}