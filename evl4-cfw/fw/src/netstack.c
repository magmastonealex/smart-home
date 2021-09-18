#include <avr/pgmspace.h>
#include "netstack.h"
#include "nic.h"
#include "timer.h"
#include "arp.h"
#include "netcommon.h"
#include "ip.h"
#include "dbgserial.h"

sk_buff inbuf;

void print_mac(uint8_t *mac) {
	for (uint8_t i = 0; i < 5; i++) {
		printf("%x:", mac[i]);
	}
	printf("%x", mac[5]);
}

void print_ip(uint32_t ip) {
	printf("%u.%u.%u.%u", (uint8_t)(ip >>0),(uint8_t)(ip >>8),(uint8_t)(ip >>16),(uint8_t)(ip >>24));
}


void netstack_init() {
    nic_init();
}

static uint16_t last_tick = 0;

void netstack_loop() {
    uint16_t res = nic_poll(NET_MTU, inbuf.buff);
    if (res > 0) {
        inbuf.len=res;
        // Got a packet!
        // Print out details...
        ether_hdr *hdr = (ether_hdr *) inbuf.buff;
        inbuf.ethhdr = hdr;
        
        uint16_t lether = ntohs(hdr->ethertype); 
        if (lether == ETH_P_ARP) {
            arp_in(&inbuf);
        } else if (lether == ETH_P_IPV4) {
            ip_recv(&inbuf);
        } else {
            DBGprintf("[ETH] unknown ethertype %04x", lether);
        }

    }

    if (rtc_get_ticks() != last_tick) {
        last_tick = rtc_get_ticks();
        // 1 second has passed. Some things need kicking.
        arp_interval();
    }
}
