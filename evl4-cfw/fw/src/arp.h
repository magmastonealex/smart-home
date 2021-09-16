#pragma once

#include "netcommon.h"

void arp_in(sk_buff *buf);
// call arp_interval at ~1 second intervals, to do cache work and 
// try to update incomplete entries.
void arp_interval();

void arp_fill(uint8_t *hwaddr, uint32_t ipaddr);

void arp_debug_dump();


void arp_fill(uint8_t *hwaddr, uint32_t ipaddr);