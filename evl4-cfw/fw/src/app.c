#include "app.h"
#include "udp.h"
#include "dbgserial.h"

void app_udp_handler(void *data, sk_buff *buf) {
    if (buf->len - UDP_PKT_START > 3) {
        DBGprintf("Got packet! %c %c %c\n", buf->udpdata[0],buf->udpdata[1],buf->udpdata[2]);
        buf->len = UDP_PKT_START + 3;
        (buf->buff + UDP_PKT_START)[0] = 'a';
        (buf->buff + UDP_PKT_START)[1] = 'b';
        (buf->buff + UDP_PKT_START)[2] = 'c';
        udp_sendto(buf->iphdr->src, 12345, ntohs(buf->udphdr->sport), buf);
        DBGprintf("sent udp resp\n");
    } else {
        DBGprintf("Got small packet!\n");
    }
}

void init_app() {
    udp_register(12345, NULL, app_udp_handler);
}

void app_periodic() {
    
}