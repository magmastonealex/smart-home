#include "checksum.h"


// Compute an IPv4 style checksum.
// Borrowed from uip,
// Copyright (c) 2001-2003, Adam Dunkels.
// licence 3-clause BSD.
uint16_t checksum_ipstyle(uint8_t *data, uint16_t len) {
    uint16_t sum = 0;
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