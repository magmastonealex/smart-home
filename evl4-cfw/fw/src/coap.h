#pragma once
#include <stdint.h>
#include "netcommon.h"

// Only 10 options per packet (they take up lots of RAM!)
#define MAX_COAP_OPTIONS 10

#define COAP_ERR_NONE 0
#define COAP_ERR_HEADER_SHORT 1
#define COAP_ERR_VERSION 2
#define COAP_ERR_TOKEN_LENGTH 3
#define COAP_ERR_OPTION_DELTA_INVALID 4
#define COAP_ERR_TOO_MANY_OPTIONS 5
#define COAP_ERR_OPTION_OVERFLOW 6
#define COAP_ERR_OPTION_PL_OVERFLOW 7
#define COAP_ERR_PAYLOAD_OVERFLOW 8

#define COAP_TYPE_CONFIRMABLE 0
#define COAP_TYPE_NON_CONFIRMABLE 1
#define COAP_TYPE_ACK 2
#define COAP_TYPE_RESET 2

#define COAP_OPTION_NO_OPTION 0
#define COAP_OPTION_URI_PATH 11
#define COAP_OPTION_CONTENT_FORMAT 12

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


void dump_coap(coap_pkt *pkt);
void dump_coap_buffer(coap_buffer *buffer);

uint8_t coap_parse(coap_pkt *pkt, sk_buff*buf);
uint8_t coap_serialize(coap_pkt *pkt, uint8_t *buf, uint16_t len, uint16_t *realLen);