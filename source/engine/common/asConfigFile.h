#ifndef _ASCONFIGFILE_H_
#define _ASCONFIGFILE_H_

#ifdef __cplusplus 
extern "C" { 
#endif

/*Todo: OH GOD WHAT WAS I THINKING, API IS A MESS... NEEDLESS ABSTRACTION... AHHH*/

#include "asCommon.h"

/**
* @brief Handle to read a config file
*/
typedef struct asCfgFile_t asCfgFile_t;
/**
* @brief Load config file from disk
* @warning allocates memory on heap, make sure to asCfgFree()
*/
ASEXPORT asCfgFile_t* asCfgLoad(const char* pPath);
/**
* @brief Load config file from user data
* @warning allocates memory on heap, make sure to asCfgFree()
*/
ASEXPORT asCfgFile_t* asCfgLoadUserFile(const char* pPath);
/**
* @brief Load config file from memory
* @warning allocates memory on heap, make sure to asCfgFree()
*/
ASEXPORT asCfgFile_t* asCfgFromMem(unsigned char* pData, size_t size);
/**
* @brief Create blank config file
*/
ASEXPORT asCfgFile_t* asCfgCreateBlank();
/**
* @brief Free the config file
*/
ASEXPORT void asCfgFree(asCfgFile_t* pCfg);
/**
* @brief Get underlying Mattias Ini Handle (BREAK OUT OF ABSTRACTION)
*/
ASEXPORT struct ini_t* asCfgGetMattiasPtr(asCfgFile_t* pCfg);
/**
* @brief Attempt to open a section from the config file by index
*/
ASEXPORT int asCfgOpenSectionByIndex(asCfgFile_t* pCfg, int index);
/**
* @brief Attempt to open a section from the config file
*/
ASEXPORT int asCfgOpenSection(asCfgFile_t* pCfg, const char* pName);
/**
* @brief Attempt to open the next section
*/
ASEXPORT int asCfgOpenNextSection(asCfgFile_t* pCfg);
/**
* @brief Attempt to open a section from the config file
* @warning when the config file is freed the string is no longer valid
*/
ASEXPORT const char* asCfgGetCurrentSectionName(asCfgFile_t* pCfg);
/**
* @brief Get section index
*/
ASEXPORT int asCfgGetSectionIndex(asCfgFile_t* pCfg);
/**
* @brief Get prop index
*/
ASEXPORT int asCfgGetPropIndex(asCfgFile_t* pCfg);
/**
* @brief Read a number from the config file
*/
ASEXPORT double asCfgGetNumber(asCfgFile_t* pCfg, const char* pName, double fallback);
/**
* @brief Read a string from the config file
* @warning when the config file is freed the string is no longer valid 
*/
ASEXPORT const char* asCfgGetString(asCfgFile_t* pCfg, const char* pName, const char* pFallback);
/**
* @brief Read a string from the config file
* @warning when the config file is freed the string is no longer valid
*/
ASEXPORT int asCfgGetPropertyAtSectionIndex(asCfgFile_t* pCfg, int sectionIdx, int index, const char** ppName, const char** ppValue);

/**
* @brief Read the next property from the config file
* @warning when the config file is freed the strings are no longer valid
* returns 0 when no more properties are found in the section
*/
ASEXPORT int asCfgGetNextProp(asCfgFile_t* pCfg, const char** ppName, const char** ppValue);

#ifdef __cplusplus
}
#endif
#endif