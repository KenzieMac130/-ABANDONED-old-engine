#include "asInput.h"

#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>

struct bindMapping { int32_t idx; float scale; };
#define INPUT_BINDING_TABLE(_keyTypeL, _name) struct { _keyTypeL key; struct bindMapping value; }* _name

struct asInputPlayerT
{
	int32_t playerIdx;
	bool ownsKeyboardMouse;
	SDL_JoystickID sdlJoystickId;
	SDL_GameController* sdlController;

	/*Bindings to Inputs*/
	INPUT_BINDING_TABLE(SDL_Scancode, bindingsKeys);
	INPUT_BINDING_TABLE(int8_t, bindingsMouseButton);
	INPUT_BINDING_TABLE(int8_t, bindingsMouseAxis);
	INPUT_BINDING_TABLE(SDL_GameControllerAxis, bindingsControllerAxis);
	INPUT_BINDING_TABLE(SDL_GameControllerButton, bindingsControllerButtons);

	/*Input Mappings*/
	size_t inputCount;
	struct { char* key; asInputMappingDesc value; }* mappingDescs;
	struct { float prev; float current; }* inputValues;
};
asInputPlayer keyboardMouseOwningPlayer = NULL;
size_t inputPlayerCount = 0;
struct asInputPlayerT inputPlayerList[AS_MAX_LOCAL_PLAYERS];

#define PLAYER_UPDATE_ITTER()\
for (int _i = 0; _i < AS_MAX_LOCAL_PLAYERS; _i++)\
{\
	struct asInputPlayerT* player = &inputPlayerList[_i];\
	if (player->playerIdx < 0) { continue; }

const uint8_t* sdlKeyState;
bool keyMouseFocus = false;
ASEXPORT asResults asInputOverrideKeyMouseNextFrame(bool override)
{
	keyMouseFocus = !override;
	return AS_SUCCESS;
}

ASEXPORT asResults asInputMouseSetRelative(bool relative)
{
	SDL_SetRelativeMouseMode((SDL_bool)relative);
}

ASEXPORT bool asInputMouseGetRelative()
{
	return (bool)SDL_GetRelativeMouseMode();
}

typedef enum mouseAxis {
	MOUSEAXIS_X,
	MOUSEAXIS_Y,
	MOUSEAXIS_SCROLLX,
	MOUSEAXIS_SCROLLY
};

int8_t mouseAxisFromString(const char* name)
{
	if (!name) { return -1; }
	if (asIsStringEqual(name, "X"))
	{
		return MOUSEAXIS_X;
	}
	else if (asIsStringEqual(name, "Y"))
	{
		return MOUSEAXIS_Y;
	}
	else if (asIsStringEqual(name, "ScrollX"))
	{
		return MOUSEAXIS_SCROLLX;
	}
	else if (asIsStringEqual(name, "ScrollY"))
	{
		return MOUSEAXIS_SCROLLY;
	}
	return -1;
}

int8_t mouseButtonFromString(const char* name)
{
	if (!name) { return -1; }
	if (asIsStringEqual(name, "Left"))
	{
		return SDL_BUTTON_LEFT;
	}
	else if (asIsStringEqual(name, "Right"))
	{
		return SDL_BUTTON_RIGHT;
	}
	else if (asIsStringEqual(name, "Middle"))
	{
		return SDL_BUTTON_MIDDLE;
	}
	else if (asIsStringEqual(name, "X1"))
	{
		return SDL_BUTTON_X1;
	}
	else if (asIsStringEqual(name, "X2"))
	{
		return SDL_BUTTON_X2;
	}
	return -1;
}

ASEXPORT asResults asInputSystemNextFrame()
{
	/*Swap Frame*/
	PLAYER_UPDATE_ITTER()
		for (int i = 0; i < arrlen(player->inputValues); i++) {
			player->inputValues[i].prev = player->inputValues[i].current;
			player->inputValues[i].current = 0;
		}
	}

	/*Update Keyboard Mouse*/
	if (keyMouseFocus && keyboardMouseOwningPlayer)
	{
		const asInputPlayer player = keyboardMouseOwningPlayer;
		/*Keyboard*/
		for (int i = 0; i < hmlen(player->bindingsKeys); i++)
		{
			if (sdlKeyState[player->bindingsKeys[i].key]) {
				const struct bindMapping mapping = player->bindingsKeys[i].value;
				player->inputValues[mapping.idx].current += mapping.scale;
			}
		}
		/*Mouse Buttons*/
		int32_t x, y;
		uint32_t mb = SDL_GetRelativeMouseState(&x, &y);
		for (int i = 0; i < hmlen(player->bindingsMouseButton); i++)
		{
			if (mb & SDL_BUTTON(player->bindingsMouseButton[i].key)) {
				const struct bindMapping mapping = player->bindingsMouseButton[i].value;
				player->inputValues[mapping.idx].current += mapping.scale;
			}
		}
		/*Mouse Relative Movement*/
		SDL_Window* mouseWindow = SDL_GetMouseFocus();
		if (mouseWindow && asInputMouseGetRelative())
		{
			int32_t width, height;
			SDL_GetWindowSize(mouseWindow, &width, &height);
			for (int i = 0; i < hmlen(player->bindingsMouseAxis); i++)
			{
				const struct bindMapping mapping = player->bindingsMouseAxis[i].value;
				float value = 0.0f;
				if (player->bindingsMouseAxis[i].key == MOUSEAXIS_X) {
					value = (float)x * width;
				}
				if (player->bindingsMouseAxis[i].key == MOUSEAXIS_Y) {
					value = (float)y * height;
				}

				player->inputValues[mapping.idx].current += mapping.scale * value;
			}
		}
	}

	return AS_SUCCESS;
}

ASEXPORT asResults asInputCreatePlayer(asInputPlayer* pPlayer, asInputPlayerDesc* pDesc)
{
	ASASSERT(pDesc->pInputMappings);
	struct asInputPlayerT player = { 0 };

	player.playerIdx = inputPlayerCount;

	arrsetlen(player.inputValues, pDesc->inputMappingCount);
	memset(player.inputValues, 0, pDesc->inputMappingCount * sizeof(player.inputValues[0]));
	player.inputCount = pDesc->inputMappingCount;

	/*Add Mappings*/
	for (int i = 0; i < pDesc->inputMappingCount; i++)
	{
		shput(player.mappingDescs, pDesc->pInputMappings[i].name, pDesc->pInputMappings[i]);
		const asInputMappingDesc* pMap = &player.mappingDescs[i].value;

		SDL_Scancode keyPos = SDL_GetScancodeFromName(pMap->keyPostivie);
		SDL_Scancode keyNeg = SDL_GetScancodeFromName(pMap->keyNegative);
		if (keyPos != SDL_SCANCODE_UNKNOWN) {
			struct bindMapping bind = { i, 1.0f };
			hmput(player.bindingsKeys, keyPos, bind);
		}
		if (keyNeg != SDL_SCANCODE_UNKNOWN) {
			struct bindMapping bind = { i, -1.0f };
			hmput(player.bindingsKeys, keyNeg, bind);
		}

		int8_t mbPos = mouseButtonFromString(pMap->mouseButtonPositive);
		int8_t mbNeg = mouseButtonFromString(pMap->mouseButtonNegative);
		if (mbPos >= 0)
		{
			struct bindMapping bind = { i, 1.0f };
			hmput(player.bindingsMouseButton, mbPos, bind);
		}
		if (mbNeg >= 0)
		{
			struct bindMapping bind = { i, -1.0f };
			hmput(player.bindingsMouseButton, mbNeg, bind);
		}

		int8_t mAxis = mouseAxisFromString(pMap->mouseAxis);
		if (mAxis >= 0)
		{
			struct bindMapping bind = { i, 1.0f };
			hmput(player.bindingsMouseAxis, mAxis, bind);
		}

		SDL_GameControllerAxis cAxis = SDL_GameControllerGetAxisFromString(pMap->controllerAxis);
		if (cAxis != SDL_CONTROLLER_AXIS_INVALID) {
			struct bindMapping bind = { i, 1.0f };
			hmput(player.bindingsControllerAxis, cAxis, bind);
		}

		SDL_GameControllerButton cbPos = SDL_GameControllerGetAxisFromString(pMap->controllerAxis);
		SDL_GameControllerButton cbNeg = SDL_GameControllerGetAxisFromString(pMap->controllerAxis);
		if (cbPos != SDL_CONTROLLER_BUTTON_INVALID) {
			struct bindMapping bind = { i, 1.0f };
			hmput(player.bindingsControllerAxis, cbPos, bind);
		}
		if (cbNeg != SDL_CONTROLLER_BUTTON_INVALID) {
			struct bindMapping bind = { i, -1.0f };
			hmput(player.bindingsControllerAxis, cbNeg, bind);
		}
	}

	/*Add Inputs*/
	player.ownsKeyboardMouse = pDesc->ownsKeyboardMouse;
	inputPlayerList[inputPlayerCount] = player;
	*pPlayer = &inputPlayerList[inputPlayerCount];
	if (pDesc->ownsKeyboardMouse) {
		keyboardMouseOwningPlayer = *pPlayer;
	};
	inputPlayerCount++;

	return AS_SUCCESS;
}

ASEXPORT asResults asInputDeletePlayer(asInputPlayer pPlayer)
{
	if (!pPlayer) { return AS_SUCCESS; }
	struct asInputPlayerT* player = pPlayer;

	arrfree(player->inputValues);

	/*Free Hashmaps*/
	hmfree(player->bindingsKeys);
	hmfree(player->bindingsMouseButton);
	hmfree(player->bindingsMouseAxis);
	hmfree(player->bindingsControllerAxis);
	hmfree(player->bindingsControllerButtons);
	shfree(player->mappingDescs);

	inputPlayerList[pPlayer->playerIdx].playerIdx = -1;

	return AS_SUCCESS;
}

ASEXPORT asInputMappingHandle asInputGetMappingHandle(asInputPlayer player, const char* inputName)
{
	int idx = shgeti(player->mappingDescs, inputName);
	return (asInputMappingHandle){ player->playerIdx, idx };
}

#define VALIDATE_INPUT(player, mapping, _return)\
 if (mapping.inputIdx < 0 ||\
player->playerIdx != mapping.playerIdx ||\
mapping.inputIdx >= player->inputCount) { return _return; }

ASEXPORT float asInputGetAxis(asInputPlayer player, asInputMappingHandle mapping)
{
	VALIDATE_INPUT(player, mapping, 0.0f)
	return player->inputValues[mapping.inputIdx].current;
}

ASEXPORT float asInputGetAxisClamped(asInputPlayer player, asInputMappingHandle mapping)
{
	const float val = asInputGetAxis(player, mapping);
	return val > 1.0f ? 1.0f : val < -1.0f ? -1.0f : val;
}

ASEXPORT bool asInputGetButton(asInputPlayer player, asInputMappingHandle mapping)
{
	VALIDATE_INPUT(player, mapping, false)
	return player->inputValues[mapping.inputIdx].current ? true : false;
}

ASEXPORT bool asInputGetButtonDown(asInputPlayer player, asInputMappingHandle mapping)
{
	VALIDATE_INPUT(player, mapping, false)
	return (player->inputValues[mapping.inputIdx].current != 0.0f
		&& player->inputValues[mapping.inputIdx].prev == 0.0f);
}

ASEXPORT bool asInputGetButtonUp(asInputPlayer player, asInputMappingHandle mapping)
{
	VALIDATE_INPUT(player, mapping, false)
	return (player->inputValues[mapping.inputIdx].current == 0.0f
			&& player->inputValues[mapping.inputIdx].prev != 0.0f);
}

ASEXPORT asResults asInitInputSystem()
{
	sdlKeyState = SDL_GetKeyboardState(NULL);
	for (int i = 0; i < AS_MAX_LOCAL_PLAYERS; i++)
	{
		inputPlayerList[i].playerIdx = -1;
	}
	return AS_SUCCESS;
}

ASEXPORT asResults asShutdownInputSystem()
{
	for (int i = 0; i < AS_MAX_LOCAL_PLAYERS; i++)
	{
		if (inputPlayerList[i].playerIdx < 0) { continue; }
		asInputDeletePlayer(&inputPlayerList[i]);
	}
	return AS_SUCCESS;
}