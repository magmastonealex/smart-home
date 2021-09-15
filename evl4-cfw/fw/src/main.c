#define F_CPU 32000000

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "dbgserial.h"
#include "nic.h"

uint8_t PACKETBUFFER[1514];
const uint8_t MACADDR[6] PROGMEM = {0x00, 0x1C, 0x2A, 0x03, 0x2F, 0xF0};

#define Le16(b)                        \
   (  ((uint16_t)(     (b) &   0xFF) << 8)  \
   |  (     ((uint16_t)(b) & 0xFF00) >> 8)  \
   )
#define Le32(b)                             \
   (  ((uint32_t)(     (b) &       0xFF) << 24)  \
   |  ((uint32_t)((uint16_t)(b) &     0xFF00) <<  8)  \
   |  (     ((uint32_t)(b) &   0xFF0000) >>  8)  \
   |  (     ((uint32_t)(b) & 0xFF000000) >> 24)  \
   )

#  define htons(a)    Le16(a)
#define ntohs(a)    htons(a)
#  define htonl(a)    Le32(a)
#define ntohl(a)    htonl(a)

#define ETH_P_ARP 0x0806 /* Address Resolution packet */
#define ETH_P_IPV4 0x0800 /* IP */
#define ARP_HTYPE_ETHER 1  /* Ethernet ARP type */
#define ETH_ALEN 6
#define IP_ALEN 4


#define IPADDR_FROM_OCTETS(a,b,c,d) (uint32_t) (((((uint32_t)a) << 0)) | ((((uint32_t)b) << 8)) | ((((uint32_t)c) << 16)) | ((((uint32_t)d) << 24)))


typedef struct {
   uint8_t dest_addr[ETH_ALEN]; /* Destination hardware address */
   uint8_t src_addr[ETH_ALEN];  /* Source hardware address */
   uint16_t ethertype;   /* Ethernet frame type */
} __attribute__((packed)) ether_hdr;

typedef struct {
   uint16_t htype;   /* Format of hardware address */
   uint16_t ptype;   /* Format of protocol address */
   uint8_t hlen;    /* Length of hardware address */
   uint8_t plen;    /* Length of protocol address */
   uint16_t op;    /* ARP opcode (command) */
   uint8_t sha[ETH_ALEN];  /* Sender hardware address */
   uint32_t spa;   /* Sender IP address */
   uint8_t tha[ETH_ALEN];  /* Target hardware address */
   uint32_t tpa;   /* Target IP address */
} __attribute__((packed)) arp_ether_ipv4;

static void print_mac(uint8_t *mac) {
	for (uint8_t i = 0; i < 5; i++) {
		printf("%x:", mac[i]);
	}
	printf("%x", mac[5]);
}

static void print_ip(uint32_t ip) {
	printf("%u.%u.%u.%u", (uint8_t)(ip >>0),(uint8_t)(ip >>8),(uint8_t)(ip >>16),(uint8_t)(ip >>24));
}

int main() {
	PORTD.DIR |= (1<<1) | (1<<0);
	PORTD.OUT |= (1<<0);
	PORTB.DIR |= (1<<0);
	PORTB.OUT |= (1<<0);
	// Switch to 32MHz operation.
	// Enable 32Mhz & 32.768KHz clocks.
	OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;
	// Wait for 32Mhz and 23Khz clocks to stabilize...
	while(!((OSC.STATUS & OSC_RC32MRDY_bm) && (OSC.STATUS & OSC_RC32KRDY_bm)));
	// Enable DFLL to calibrate 32Mhz clock.
	DFLLRC32M.CTRL |= (1<<0);

	// And switch to 32MHz clock.
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;

	init_serial();
	DBGprintf("hello world\n");
	nic_init();

	//PORTD.OUT |= (1<<3);
	uint8_t i = 0;
	while(1) {
		i++;
		_delay_ms(50);
		PORTD.OUT |= (1<<1);
		_delay_ms(50);
		PORTD.OUT &= ~(1<<1);
		uint16_t res = nic_poll(1514, PACKETBUFFER);
		if (res > 0) {
			// Got a packet!
			// Print out details...
			DBGprintf("Packet? %d\n", res);
			ether_hdr *hdr = (ether_hdr *) PACKETBUFFER;
			DBGprintf("dest: ");
			print_mac(hdr->dest_addr);
			DBGprintf(" src: ");
			print_mac(hdr->src_addr);
			DBGprintf(" ethertype: %04x datalen %" PRIu16 "\n", ntohs(hdr->ethertype), res-sizeof(ether_hdr));

			uint16_t lether = ntohs(hdr->ethertype); 
			if (lether == ETH_P_ARP) {
				DBGprintf("got ARP\n");
				if (res < (sizeof(ether_hdr) + sizeof(arp_ether_ipv4))) {
					DBGprintf("bad packet\n");
					continue;
				}
				arp_ether_ipv4 *arp = (arp_ether_ipv4*) (PACKETBUFFER + sizeof(ether_hdr));
				DBGprintf("spa: ");
				print_ip(arp->spa);
				DBGprintf(" tpa: ");
				print_ip(arp->tpa);
				DBGprintf("\n");
			}
		}
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
		}
	}
}
