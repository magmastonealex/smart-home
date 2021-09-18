#include "udp.h"
#include "checksum.h"
#include "dbgserial.h"
#include "ip.h"

typedef struct {
    void *data;
    udp_handler handler;
    uint16_t dport;
} udp_handler_registration;

#define MAX_UDP_HANDLERS 2

static udp_handler_registration udp_handlers[MAX_UDP_HANDLERS] = {0};

void udp_register(uint16_t dport, void* data, udp_handler handler) {
    // find empty slot.
    for (uint8_t i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (udp_handlers[i].dport == 0) {
            udp_handlers[i].dport = dport;
            udp_handlers[i].handler = handler;
            udp_handlers[i].data = data;
            return;
        }
    }

    // Well, this is bad. Someone added more apps without increasing MAX_UDP_HANDLERS.
    DBGprintf("[UDP] full %u", dport);
}

void udp_recv(sk_buff *buf) {
    if (buf->len < (sizeof(ether_hdr) + sizeof(ip4_hdr) + sizeof(udp_hdr))) {
        return;
    }
    udp_hdr *hdr = (udp_hdr*) (buf->buff +(sizeof(ether_hdr) + sizeof(ip4_hdr)));
    buf->udphdr = hdr;
    buf->udpdata = buf->buff+UDP_PKT_START;

    // check for udp length sanity, otherwise drop.
    if (buf->len < (sizeof(ether_hdr) + sizeof(ip4_hdr) + ntohs(hdr->len))) {
        return;
    }

    if (hdr->checksum != 0x0000) {
        uint16_t chsum = checksum_udpstyle(buf->iphdr, buf->udphdr, buf->buff + (sizeof(ether_hdr) + sizeof(ip4_hdr)), buf->len - (sizeof(ether_hdr) + sizeof(ip4_hdr)));
        if (0xFFFF != chsum) {
            return;
        }
    }

    if (hdr->dport == 0) {
        // reserved, not used.
        return;
    }

    uint16_t dport = ntohs(hdr->dport);
    // Look for a handler for this port...
    for (uint8_t i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (udp_handlers[i].dport == dport) {
            udp_handlers[i].handler(udp_handlers[i].data, buf);
            return;
        }
    }
    DBGprintf("[UDP] unknport: %u", dport);

}

// send this packet to a destintion.
// UDP + IP + ETH will write to the first sizeof(udp_hdr) + sizeof(ip4_hdr) + sizeof(ether_hdr) bytes, so anything
// you put there will be clobbered. Start your packet after that.
uint8_t udp_sendto(uint32_t dst, uint16_t sport, uint16_t dport, sk_buff *buf) {
    if (buf->len < (sizeof(ether_hdr) + sizeof(ip4_hdr)+sizeof(udp_hdr))) {
        DBGprintf("UDP2small\n");
    }

    // Used for UDP checksum calculation.
    ip4_hdr *iphdr = (ip4_hdr*) (buf->buff +(sizeof(ether_hdr)));
    buf->iphdr = iphdr;
    iphdr->src = ip_get_ip_info()->addr;
    iphdr->dst = dst;

    udp_hdr *hdr = (udp_hdr*) (buf->buff +(sizeof(ether_hdr) + sizeof(ip4_hdr)));
    buf->udphdr = hdr;
    hdr->sport = htons(sport);
    hdr->dport = htons(dport);
    hdr->len = htons(buf->len - (sizeof(ether_hdr) + sizeof(ip4_hdr)));
    hdr->checksum = 0x00;
    hdr->checksum = htons(checksum_udpstyle(buf->iphdr, buf->udphdr, buf->buff + (sizeof(ether_hdr) + sizeof(ip4_hdr)), buf->len - (sizeof(ether_hdr) + sizeof(ip4_hdr))) ^ 0xFFFF);
    
    return ip_sendto(dst, IP_PROTO_UDP, buf);
}