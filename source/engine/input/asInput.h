#ifndef _ASINPUT_H_
#define _ASINPUT_H_

#include "../engine/common/asCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AS_MAX_LOCAL_PLAYERS 32

typedef struct { int32_t playerIdx; int32_t inputIdx; } asInputMappingHandle;
typedef struct asInputPlayerT* asInputPlayer;

ASEXPORT asResults asInitInputSystem();
ASEXPORT asResults asShutdownInputSystem();
ASEXPORT asResults asInputSystemNextFrame();

ASEXPORT asResults asInputOverrideKeyMouseNextFrame(bool override);

ASEXPORT asResults asInputMouseSetRelative(bool relative);
ASEXPORT bool asInputMouseGetRelative();

/**
* @breif description for input mappings
* keep these strings alive for app's lifetime
*/
typedef struct {
	const char* name;
	const char* keyPostivie;
	const char* keyNegative;
	const char* mouseAxis;
	const char* mouseButtonPositive;
	const char* mouseButtonNegative;
	const char* controllerAxis;
	const char* controllerButtonPositive;
	const char* controllerButtonNegative;
} asInputMappingDesc;

typedef struct {
	char* playerConfigFileName;
	bool ownsKeyboardMouse;
	int controllerIndex;
	size_t inputMappingCount;
	asInputMappingDesc* pInputMappings;
} asInputPlayerDesc;

ASEXPORT asResults asInputCreatePlayer(asInputPlayer* pPlayer, asInputPlayerDesc* pDesc);
ASEXPORT asResults asInputDeletePlayer(asInputPlayer player);

ASEXPORT asInputMappingHandle asInputGetMappingHandle(asInputPlayer player, const char* inputName);

ASEXPORT float asInputGetAxis(asInputPlayer player, asInputMappingHandle mapping);
ASEXPORT float asInputGetAxisClamped(asInputPlayer player, asInputMappingHandle mapping);

ASEXPORT bool asInputGetButton(asInputPlayer player, asInputMappingHandle mapping);
ASEXPORT bool asInputGetButtonDown(asInputPlayer player, asInputMappingHandle mapping);
ASEXPORT bool asInputGetButtonUp(asInputPlayer player, asInputMappingHandle mapping);

#ifdef __cplusplus
}
#endif
#endif