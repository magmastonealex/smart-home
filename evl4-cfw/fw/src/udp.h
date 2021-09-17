#pragma once
#include <stdint.h>
#include "netcommon.h"

// Receive a UDP packet,
// and farm it out to any listeners.
// If nothing is listening, a ICMP Port Unreachable message will be sent.
void udp_recv(sk_buff *buf);

// send this packet to a destintion.
// UDP + IP + ETH will write to the first sizeof(udp_hdr) + sizeof(ip4_hdr) + sizeof(ether_hdr) bytes, so anything
// you put there will be clobbered. Start your packet after that.
uint8_t udp_sendto(uint32_t dst, uint16_t sport, uint16_t dport, sk_buff *buf);