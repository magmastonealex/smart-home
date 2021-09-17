#include "icmp.h"
#include "ip.h"
#include "dbgserial.h"
#include "checksum.h"

// Receive an ICMP packet.
// Most of these we don't do much with other than log,
// but we can respond to pings!
void icmp_recv(sk_buff *buf) {
    if (buf->len < (sizeof(ether_hdr) + sizeof(ip4_hdr) + sizeof(icmp_hdr))) {
        DBGprintf("[ICMP] bad packet - too small\n");
        return;
    }

    icmp_hdr * hdr = (icmp_hdr *) (buf->plstart);
    uint16_t chsum = htons(checksum_ipstyle(buf->plstart, buf->len - (buf->plstart-buf->buff)));
    if (0xFFFF != chsum) {
        DBGprintf("[ICMP] checksum mismatch r:%04x c:%04x\n", hdr->checksum, chsum);
        return;
    }
    
    if (hdr->type == 8) {
        // Echo req. Let's reply!
        // We're doing this carefully - we'll re-write the icmp header, then send it
        // right back down the stack. Avoids pointless copying of payloads.
        hdr->type = 0; // Echo reply;
        hdr->checksum = 0;
        hdr->checksum = htons(checksum_ipstyle(buf->plstart, buf->len - (buf->plstart-buf->buff))) ^ 0xFFFF;
        ip_sendto(buf->iphdr->src, IP_PROTO_ICMP, buf);
    } else {
        DBGprintf("[ICMP] Received unknown ICMP type %u code %u", hdr->type, hdr->code);
    }
}