#include <avr/pgmspace.h>
#include "netstack.h"
#include "nic.h"
#include "timer.h"
#include "arp.h"
#include "netcommon.h"
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
        DBGprintf("Packet? %d\n", res);
        ether_hdr *hdr = (ether_hdr *) inbuf.buff;
        DBGprintf("dest: ");
        print_mac(hdr->dest_addr);
        DBGprintf(" src: ");
        print_mac(hdr->src_addr);
        DBGprintf(" ethertype: %04x datalen %" PRIu16 "\n", ntohs(hdr->ethertype), res-sizeof(ether_hdr));

        uint16_t lether = ntohs(hdr->ethertype); 
        if (lether == ETH_P_ARP) {
            arp_in(&inbuf);
        }
    }

    if (rtc_get_ticks() != last_tick) {
        last_tick = rtc_get_ticks();
        // 1 second has passed. Some things need kicking.
        arp_interval();
        if (rtc_get_ticks() % 30 == 0) {
            arp_debug_dump();
        } 
    }
		/*
		if (i > 20) {

			i = 0;
			DBGprintf("prepping arp...\n");
			ether_hdr *hdr = (ether_hdr *) PACKETBUFFER;
			memset(PACKETBUFFER, 0x00, 60);

			memset(hdr->dest_addr, 0xFF, 6);
			memcpy_P(hdr->src_addr, MACADDR, 6);
			hdr->ethertype = htons(ETH_P_ARP);
			arp_ether_ipv4 *arp = (arp_ether_ipv4*) (PACKETBUFFER + sizeof(ether_hdr));
			arp->htype = htons(ARP_HTYPE_ETHER);
			arp->ptype = htons(ETH_P_IPV4);
			arp->hlen = ETH_ALEN;
			arp->plen = IP_ALEN;
			arp->op = htons(1);
			memcpy_P(arp->sha, MACADDR, 6);
			memset(arp->tha, 0xFF, 6);
			arp->spa = IPADDR_FROM_OCTETS(10, 102, 40, 253);
			arp->tpa = IPADDR_FROM_OCTETS(10, 102, 40, 128);
			DBGprintf("sending arp...\n");
			nic_send(60, PACKETBUFFER);
			DBGprintf("sent\n");
		}*/
}