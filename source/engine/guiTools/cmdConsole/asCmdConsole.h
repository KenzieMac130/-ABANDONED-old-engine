#ifndef _ASCMDCONSOLE_H_
#define _ASCMDCONSOLE_H_

#include "../../common/asCommon.h"
#include "../../common/preferences/asPreferences.h"

#ifdef __cplusplus
extern "C" {
#endif 

/*Register Pref Manager to Namespace*/
ASEXPORT asResults asGuiToolCommandConsole_RegisterPrefManager(asPreferenceManager* manager, const char* nameSpace);

/*Immediate GUI Widget*/
ASEXPORT void asGuiToolCommandConsoleUI();

#ifdef __cplusplus
}
#endif
#endif