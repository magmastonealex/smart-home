#include "ip.h"
#include "netcommon.h"
#include "checksum.h"
#include "dbgserial.h"
#include "icmp.h"
#include "arp.h"
#include "nic.h"
#include "udp.h"

#include <string.h>

static uint16_t datagram_count = 0;

net_ip_info info = {IPADDR_FROM_OCTETS(192, 168, 50, 30), IPADDR_FROM_OCTETS(255,255,255,255), IPADDR_FROM_OCTETS(192, 168, 50, 1)};

net_ip_info *ip_get_ip_info() {
    return &info;
}

// Receive an IP packet, and send it up the stack to UDP/ICMP.
void ip_recv(sk_buff *buf) {
    // Check the checksum and drop invalid packets.
    if (buf->len < (sizeof(ether_hdr) + sizeof(ip4_hdr))) {
        return;
    }

    ip4_hdr *hdr = (ip4_hdr*) (buf->buff + sizeof(ether_hdr));
    buf->iphdr = hdr;

    uint16_t iphdrlen = 4*(hdr->ver_len & 0x0F);
    if (buf->len < (sizeof(ether_hdr) + iphdrlen)) {
        return;
    }

    uint16_t chsum = checksum_ipstyle(buf->buff + sizeof(ether_hdr), iphdrlen);
    if (0xFFFF != chsum) {
        return;
    }

    if ((hdr->ver_len & 0xF0) != 0x40) {
        return;
    }

    // check src address (no broadcast/multicast support, yet.)
    if (hdr->dst != info.addr) {
        return;
    }

    // Check flags - if fragmented, drop.
    // Realistically with our small MTU they never get here.
    if ((hdr->flags_fragment & 0x00E0) & 0x20) {
        return;
    }

    // Options are ignored entirely. No one actually uses these, right? :) (I know this is bad, but this is my IP stack, so leave me alone)
    if(iphdrlen > 20) {
        if ((sizeof(ether_hdr) + iphdrlen) > buf->len) {
            return;
        }
        memmove(
            buf->buff + sizeof(ether_hdr) + sizeof(ip4_hdr),
            buf->buff + sizeof(ether_hdr) + iphdrlen,
            buf->len - (sizeof(ether_hdr) + iphdrlen));
        buf->len -= (iphdrlen-sizeof(ip4_hdr));
    }

    // Update length field. Ethernet does not allow packets smaller than 64 bytes, so very small packets will have a strange length.
    // Now we know how long the packet really was, trim it accordingly.
    uint16_t oldlen = buf->len;
    buf->len = sizeof(ether_hdr) + (ntohs(hdr->total_length));
    if (oldlen < buf->len) {
        // bad things just happened.... Set it back.
        buf->len = oldlen;
    }
    buf->plstart = buf->buff + (sizeof(ether_hdr) + sizeof(ip4_hdr));

    if(hdr->protocol == IP_PROTO_ICMP) {
        icmp_recv(buf);
    } else  if (hdr->protocol == IP_PROTO_UDP) {
        udp_recv(buf);
    } else {
        return;
    }
    
}

// send this packet to a destintion.
// IP will write to the first sizeof(ip4_hdr) + sizeof(ether_hdr) bytes, so anything
// you put there will be clobbered. Start your packet after that.
uint8_t ip_sendto(uint32_t dst, uint8_t protocol, sk_buff *buf) {
    // TODO: Handle routing.
    // If dst is not on the same subnet, send it to the _gateway_'s MAC.
    // This will kinda-sorta work now because ARP has a dumb implementation
    // not looking at subnet masks, but it doesn't let you initiate to other subnets,
    // only reply.

    if (buf->len < (sizeof(ether_hdr) + sizeof(ip4_hdr))) {
        DBGprintf("[IP] bad send 2small\n");
    }
    memset(buf->buff, 0x00, sizeof(ether_hdr) + sizeof(ip4_hdr));
    ether_hdr *ethhdr = (ether_hdr *)buf->buff;
    ip4_hdr *iphdr = (ip4_hdr*) (buf->buff + sizeof(ether_hdr));

    // Fill out Ethernet header.
    nic_write_mac(ethhdr->src_addr);
    arp_fill(ethhdr->dest_addr, dst);
    ethhdr->ethertype = htons(ETH_P_IPV4);

    // Fill out (barebones) IP header...
    iphdr->src = info.addr;
    iphdr->dst = dst;
    iphdr->protocol = protocol;
    iphdr->ttl = 129;
    iphdr->identification = datagram_count++;
    iphdr->ver_len = 0x45; // no options, ipv4
    iphdr->total_length = htons(buf->len - sizeof(ether_hdr));
    iphdr->checksum = htons(checksum_ipstyle(buf->buff + sizeof(ether_hdr), sizeof(ip4_hdr)) ^ 0xFFFF);

    // rest of packet is upper-layer data. Send to NIC!
    nic_send(buf);
    return 0;
}