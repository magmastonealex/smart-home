#pragma once
#include <stdint.h>
#include "netcommon.h"

uint16_t checksum_ipstyle(uint8_t *data, uint16_t length);
uint16_t checksum_udpstyle(ip4_hdr *iphdr, udp_hdr *uhdr, uint8_t *data, uint16_t len);