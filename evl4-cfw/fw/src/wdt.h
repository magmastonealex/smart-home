#pragma once

// Enable our watchdog timer at 1s.
// the alarm app kicks the watchdog every time a success response is received from the 
// i2c bus.

void watchdog_init();
void watchdog_kick();

// SW reset is triggered if one of our COAP messages doesn't get a response after 5-6 seconds.
// This is intended to be last-resort in case the NIC crashes.
void watchdog_sw_reset();