#pragma once
#include <stdint.h>

// Attempts to generate some degree of randomness using ADC readings
// from internal temperature sensor, and a rapidly shifting AREF.
uint16_t random_get();