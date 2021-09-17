#pragma once
#include <inttypes.h>

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

// We're going to define a relatively low MTU here as we don't have anything data-intensive to do.
// The biggest 
#define NET_MTU 1550

// Reserve sk_buffs for applications to use - this is packets pending transmission.
// You probably want this to be roughly equal to the number of apps
// in use to avoid contention.
#define SKBUFF_APP 3 

// Reserve an SK_BUF for ARP transmissions
#define SKBUFF_ARP 1

// one SK_BUFF will be reserved for incoming packets.

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

typedef struct {
    uint8_t ver_len;
    uint8_t dscp_ecn;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
} __attribute__((packed)) ip4_hdr;

typedef struct {
    uint16_t sport;
    uint16_t dport;
    uint16_t len;
    uint16_t checksum;
} __attribute__((packed)) udp_hdr;

typedef struct {
    uint8_t flags;
    uint8_t buff[NET_MTU];
    uint16_t len;
} sk_buff;

void print_mac(uint8_t *mac);
void print_ip(uint32_t ip);