#include "asTime.h"

#include <SDL_timer.h>

/*Time*/

ASEXPORT asTimer_t asTimerStart()
{
	asTimer_t result;
	result.freq = SDL_GetPerformanceFrequency();
	result.last = SDL_GetPerformanceCounter();
	return result;
}

ASEXPORT asTimer_t asTimerRestart(asTimer_t prev)
{
	prev.last = SDL_GetPerformanceCounter();
	return prev;
}

ASEXPORT uint64_t asTimerTicksElapsed(asTimer_t timer)
{
	uint64_t now = SDL_GetPerformanceCounter();
	return now - timer.last;
}

ASEXPORT uint64_t asTimerMicroseconds(asTimer_t timer, uint64_t ticks)
{
	return ticks / (timer.freq / 1000000);
}

ASEXPORT double asTimerSeconds(asTimer_t timer, uint64_t ticks)
{
	return (double)ticks / timer.freq;
}
