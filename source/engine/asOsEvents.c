#include "include/asOsEvents.h"
#include <SDL.h>

#include "include/asEntry.h"

ASEXPORT void asPollOSEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			asExitLoop();
			break;
		default:
			break;
		}
	}
}