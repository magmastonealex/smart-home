#pragma once
#include <stdint.h>
#include "netcommon.h"

typedef struct {
    uint32_t addr;
    uint32_t nmask;
    uint32_t gw;
} net_ip_info;

net_ip_info *ip_get_ip_info();

// Receive an IP packet, and send it up the stack to UDP/TCP.
void ip_recv(sk_buff *buf);

// send this packet to a destintion.
// IP + ETH will write to the first sizeof(ip4_hdr) + sizeof(ether_hdr) bytes, so anything
// you put there will be clobbered. Start your packet after that.
uint8_t ip_sendto(uint32_t dst, uint8_t protocol, sk_buff *buf);
