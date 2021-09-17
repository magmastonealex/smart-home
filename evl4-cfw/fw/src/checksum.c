#include "checksum.h"
#include "netcommon.h"

// Compute an IPv4 style checksum.
// Borrowed from uip,
// Copyright (c) 2001-2003, Adam Dunkels.
// licence 3-clause BSD.

static uint16_t chksum(uint16_t sum, uint8_t *data, uint16_t len) {
    uint16_t t;
    const uint8_t *dataptr = data;
    const uint8_t *last_byte = data + len + 1;

    last_byte = data + len - 1; 

    while(dataptr < last_byte) {  /* At least two more bytes */
        t = (((uint16_t)dataptr[0] << 8)) + ((uint16_t)dataptr[1]);
        sum += t;
        if(sum < t) { 
            sum++;            /* carry */
        }    
        dataptr += 2;
    }

    if(dataptr == last_byte) {
        t = (dataptr[0] << 8) + 0; 
        sum += t;
        if(sum < t) { 
            sum++;            /* carry */
        }    
    }
    /* Return sum in host byte order. */
    return sum;
}

uint16_t checksum_ipstyle(uint8_t *data, uint16_t len) {
    
    return chksum(0, data, len);
}

struct udp_pseudoheader {
    uint32_t src;
    uint32_t dst;
    uint16_t protocol;
    uint16_t length;
};

uint16_t checksum_udpstyle(ip4_hdr *iphdr, udp_hdr *uhdr, uint8_t *data, uint16_t len) {
    struct udp_pseudoheader phdr;
    phdr.src = iphdr->src;
    phdr.dst = iphdr->dst;
    phdr.protocol = 0x1100;
    phdr.length = uhdr->len;

    // sum length + protocol.
    uint16_t sum = chksum(0, (uint8_t*)&phdr, sizeof(struct udp_pseudoheader));
    return chksum(sum, data, len);
}