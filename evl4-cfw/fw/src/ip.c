#include "ip.h"
#include "netcommon.h"
#include "checksum.h"
#include "dbgserial.h"
#include <string.h>

net_ip_info info = {IPADDR_FROM_OCTETS(192, 168, 50, 30), IPADDR_FROM_OCTETS(255,255,255,255), IPADDR_FROM_OCTETS(192, 168, 50, 1)};

net_ip_info *ip_get_ip_info() {
    return &info;
}

// Receive an IP packet, and send it up the stack to UDP/ICMP.
void ip_recv(sk_buff *buf) {
    // Check the checksum and drop invalid packets.
    if (buf->len < (sizeof(ether_hdr) + sizeof(ip4_hdr))) {
        DBGprintf("[IP] bad packet - too small\n");
        return;
    }

    ip4_hdr *hdr = (ip4_hdr*) (buf->buff + sizeof(ether_hdr));

    uint16_t iphdrlen = 4*(hdr->ver_len & 0x0F);
    if (buf->len < (sizeof(ether_hdr) + iphdrlen)) {
        DBGprintf("[IP] bad packet - too small for specified header\n");
        return;
    }

    uint16_t chsum = htons(checksum_ipstyle(buf->buff + sizeof(ether_hdr), iphdrlen));
    if (0xFFFF != chsum) {
        DBGprintf("[IP] checksum mismatch r:%04x c:%04x\n", hdr->checksum, chsum);
        return;
    }

    DBGprintf("[IP] from ");
    print_ip(hdr->src);
    DBGprintf(" to: ");
    print_ip(hdr->dst);
    DBGprintf(" protocol: %x\n", hdr->protocol);

    if ((hdr->ver_len & 0xF0) != 0x40) {
        DBGprintf("[IP] version not 4? %x\n", hdr->ver_len);
        return;
    }

    // check src address (no broadcast/multicast support, yet.)
    if (hdr->dst != info.addr) {
        DBGprintf("[IP] packet not addressed to us");
        print_ip(hdr->dst);
        DBGprintf("\n");
        return;
    }

    // Check flags - if fragmented, drop.
    // Realistically with our small MTU they never get here.
    if ((hdr->flags_fragment & 0x00E0) & 0x20) {
        DBGprintf("[IP] fragmented packet ignored: %04x\n", hdr->flags_fragment);
        return;
    }

    // Options are ignored entirely. No one actually uses these, right? :) (I know this is bad, but this is my IP stack, so leave me alone)
    if(iphdrlen > 20) {
        DBGprintf("[IP] options dropped Before %d", buf->len);
        if ((sizeof(ether_hdr) + iphdrlen) > buf->len) {
            DBGprintf("[IP] Bad header length. Dropping packet entirely\n");
            return;
        }
        memmove(
            buf->buff + sizeof(ether_hdr) + sizeof(ip4_hdr),
            buf->buff + sizeof(ether_hdr) + iphdrlen,
            buf->len - (sizeof(ether_hdr) + iphdrlen));
        buf->len -= (iphdrlen-sizeof(ip4_hdr));
        DBGprintf(" now %d payload %u\n", buf->len, buf->len - (sizeof(ether_hdr) + sizeof(ip4_hdr)));
    }

    if(hdr->protocol == 0x01) {
        DBGprintf("[IP] ICMP type %02x. payload size %u IP reported %u\n", (buf->buff + sizeof(ether_hdr) + sizeof(ip4_hdr))[0], buf->len - (sizeof(ether_hdr) + sizeof(ip4_hdr)), ntohs(hdr->total_length)-iphdrlen);
    } else {
        DBGprintf("[IP] unknown protocol %x. Dropping.\n", hdr->protocol);
        return;
    }
    
}

// send this packet to a destintion.
// IP + ETH will write to the first sizeof(ip4_hdr) + sizeof(ether_hdr) bytes, so anything
// you put there will be clobbered. Start your packet after that.
uint8_t ip_sendto(uint32_t dst, sk_buff *buf) {
    return 1;
}