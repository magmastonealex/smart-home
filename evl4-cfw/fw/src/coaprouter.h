#pragma once
#include <stdint.h>
// A rough-around-the-edges coap implementation, to go along with my rough-around-the-edges UDP/IP stack :)
// You probably want to just use microcoap or CoAP-simple-library or similar.
// This is purpose built to handle two (and only two) things:
// 1. Serve requests to get/set configuration & OTA updates.
// 2. Provide access to a pub (not even sub) server to publish sensor values supporting retries.
// Oh, and hopefully do all the above in only a couple hundred bytes of RAM.
// There is currently no support for GET/PUT/POST requests out to other servers,
// other than the above mentioned publish support. You could probably extend this if you really wanted to.
// something something callbacks/promises/futures.

// This is the coap-router which handles callbacks to other applications,
// and pub-sub management.
// coap.{c/h} handles serialization/deserialization.


void init_coaprouter();

void coaprouter_periodic();

void coap_update_sensor(uint8_t sensorid, uint8_t value);
void coap_mark_ready();