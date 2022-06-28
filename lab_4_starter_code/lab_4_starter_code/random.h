/**
 * Provides a method to simulate the random arrival of events that have
 * average arrival rate.  Since Cortex-M3 doesn't support floating-point
 * arithmetic, next arrival times are represented in 32-bit fixed-point
 * with 16 bits before the decimal point.
 *
 * Andrew Morton, 2018
 */
#ifndef random_h
#define random_h

#include <stdint.h>
#include "lfsr113.h"

/**
 * Generate -ln(U)*2^16 in fixed-point representation.
 */
uint32_t next_event(void);

/*
	Generate seconds to the next event
*/
uint32_t get_random_delay_seconds(uint32_t rateParameter,uint32_t tickFreq);

#endif
