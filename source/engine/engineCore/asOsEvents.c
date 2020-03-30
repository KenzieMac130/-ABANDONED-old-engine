#include "asOsEvents.h"
#include <SDL.h>

#include "asEntry.h"
#include "../renderer/asRendererCore.h"
#if ASTRENGINE_NUKLEAR
#include "../nuklear/asNuklearImplimentation.h"
#include "../thirdparty/nuklear/nuklear.h"
#endif

#if ASTRENGINE_DEARIMGUI
#include "../cimgui/asDearImGuiImplimentation.h"
#endif

ASEXPORT void asPollOSEvents()
{
#if ASTRENGINE_NUKLEAR
	nk_input_begin(asGetNuklearContextPtr());
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
#if ASTRENGINE_DEARIMGUI
		asImGuiPushEvent(&event);
#endif
	}

#if ASTRENGINE_NUKLEAR
	nk_input_end(asGetNuklearContextPtr());
#endif
}