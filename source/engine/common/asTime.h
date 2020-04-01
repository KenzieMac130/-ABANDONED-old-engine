#ifndef _ASTIME_H_
#define _ASTIME_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

/*Time*/

/**
* @brief A timer object
*/
typedef struct {
	uint64_t freq;
	uint64_t last;
} asTimer_t;

/**
* @brief Start the timer
*/
ASEXPORT asTimer_t asTimerStart();
/**
* @brief Restart the timer and return it as a new one
*/
ASEXPORT asTimer_t asTimerRestart(asTimer_t prev);

/**
* @brief Get the last time in platform dependent ticks
*/
ASEXPORT uint64_t asTimerTicksElapsed(asTimer_t timer);

/**
* @brief Get mileseconds that passed based on timer
*/
ASEXPORT uint64_t asTimerMicroseconds(asTimer_t timer, uint64_t ticks);

/**
* @brief Get seconds that passed based on timer as a double
*/
ASEXPORT double asTimerSeconds(asTimer_t timer, uint64_t ticks);

#ifdef __cplusplus
}
#endif
#endif