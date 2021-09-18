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

// Only 10 options per packet (they take up lots of RAM!)
#define MAX_COAP_OPTIONS 10

typedef struct {
    uint8_t tkl:4;
    uint8_t type:2;
    uint8_t ver:2;
    
    uint8_t code_detail:5;
    uint8_t code_class:3;
    
    uint16_t msgid_be;
}  __attribute__((packed)) coap_pkt_hdr;

typedef struct
{
    uint8_t *p;
    uint16_t len;
} coap_buffer;

typedef struct {
    uint16_t option_number;
    coap_buffer option_value;
} coap_option;

typedef struct {
    coap_pkt_hdr *hdr;
    coap_buffer token;
    coap_option options[MAX_COAP_OPTIONS];
    coap_buffer data;
} coap_pkt;

void init_coap();

void coap_periodic();