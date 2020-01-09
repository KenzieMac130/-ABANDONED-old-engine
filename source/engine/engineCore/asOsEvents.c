#include "asOsEvents.h"
#include <SDL.h>

#include "asEntry.h"
#include "../renderer/asRendererCore.h"
#if ASTRENGINE_NUKLEAR
#include "../nuklear/asNuklearImplimentation.h"
#endif

ASEXPORT void asPollOSEvents()
{
#if ASTRENGINE_NUKLEAR
#endif
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			asExitLoop();
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_MINIMIZED:
				asGfxSetDrawSkip(true);
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				asGfxSetDrawSkip(false);
				break;
			case SDL_WINDOWEVENT_RESIZED:
				asGfxTriggerResizeEvent();
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
#if ASTRENGINE_NUKLEAR
		asNkPushEvent(&event);
#endif
	}
#if ASTRENGINE_NUKLEAR

#endif
}