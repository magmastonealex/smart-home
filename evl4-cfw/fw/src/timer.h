#pragma once
#include <stdint.h>


void init_timers();

// Get a 10ms ticker value.
// This will roll over every 10 minutes -
uint16_t timer_get_ticks();

// Get 1hz tick counter.
uint16_t rtc_get_ticks();