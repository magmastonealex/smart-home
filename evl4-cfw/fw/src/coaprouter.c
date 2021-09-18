#include "coaprouter.h"
#include "udp.h"
#include "coap.h"
#include "dbgserial.h"
#include <string.h>
#include <avr/pgmspace.h>

sk_buff coapOutputBuffer = {0};

// Reservation for space to parse an incoming packet.
coap_pkt inpkt;

coap_pkt_hdr outpktHdr;
coap_pkt outpkt;

uint8_t coapScratch[100];
// Callbacks will need to set:
// resp->hdr->code_class
// resp->hdr->code_detail
// any options of interest
// a payload.
// The payload (& options) need to live on after this returns (don't put it on the stack!)
// but don't need to live on once called again.
// coapScratch is available & reccommended for this purpose.
typedef void (*coapCallback)(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf);

typedef struct {
    PGM_P path;
    uint8_t strlen;
    coapCallback callback;
    void* data;
} coap_endpoint;


const char wellknownsdresp[] PROGMEM = "</rawvalues>;title=\"current raw adc values. Returns 8 bytes, one per channel.\",\n";

void cb_servicediscovery(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf) {
    coapScratch[0] = 40; // content-type - link-format.
    uint16_t datalen = strlen_P(wellknownsdresp);
    memcpy_P(coapScratch+1, wellknownsdresp, datalen);
    res->hdr->code_class = 2;
    res->hdr->code_detail = 5;

    res->options[0].option_number = COAP_OPTION_CONTENT_FORMAT;
    res->options[0].option_value.len = 1;
    res->options[0].option_value.p = coapScratch;

    res->data.len = datalen;
    res->data.p = coapScratch+1;
}

const char endpointspath[] PROGMEM = ".endpoints";

coap_endpoint coap_endpoints[] = {
    {.path = endpointspath, .strlen = 0, .callback = cb_servicediscovery, .data = NULL}
};

void coaprouter_udp_handler(void *data, sk_buff *buf) {
    memset((uint8_t*)&inpkt, 0, sizeof(coap_pkt));
    uint8_t ret = coap_parse(&inpkt, buf);
    if (ret != 0) {
        DBGprintf("coap parse failed: %u", ret);
        return;
    }
    
    // TODO: if this is an ack for something we sent, this needs to go off somewhere else here.

    uint16_t dport = buf->udphdr->sport;
    uint32_t dst = buf->iphdr->src;

    memset((uint8_t*)&outpkt, 0, sizeof(coap_pkt));
    outpkt.hdr = &outpktHdr;

    outpkt.hdr->msgid_be = inpkt.hdr->msgid_be;
    outpkt.hdr->type = COAP_TYPE_ACK;
    outpkt.hdr->ver = 1;
    
    outpkt.token.len = inpkt.token.len;
    outpkt.token.p = inpkt.token.p;
    outpkt.hdr->tkl = outpkt.token.len;

    // check for a path option...
    uint8_t pathOption = 255;
    for (uint8_t i = 0; i < MAX_COAP_OPTIONS; i++)
    {
        if (inpkt.options[i].option_number == COAP_OPTION_URI_PATH) {
            pathOption = i;
            break;
        }
    }
    
    uint8_t handled = 0;
    if (pathOption != 255) {
        // Look for a handler...
        uint8_t numendpoints = (sizeof(coap_endpoints)/sizeof(coap_endpoint));
        for (uint8_t i = 0; i < numendpoints; i++) {
            if (inpkt.options[pathOption].option_value.len == (uint16_t)coap_endpoints[i].strlen) {
                if (memcmp_P(inpkt.options[pathOption].option_value.p, coap_endpoints[i].path, coap_endpoints[i].strlen) == 0){
                    // we have a match!
                    handled = 1;
                    coap_endpoints[i].callback(coap_endpoints[i].data, &inpkt, &outpkt, buf);
                    break;
                }
            }            
        }
    }

    if (!handled) {
        // send a 404.
        outpkt.data.len = 0;
        outpkt.hdr->code_class=4;
        outpkt.hdr->code_detail=4;
    }
    
    coapOutputBuffer.len = NET_MTU;
    uint16_t usedLen = 0;
    ret = coap_serialize(&outpkt, coapOutputBuffer.buff+UDP_PKT_START, coapOutputBuffer.len - UDP_PKT_START, &usedLen);
    if (ret != 0) {
        DBGprintf("coap serialize failed: %u", ret);
        return;
    }
    
    coapOutputBuffer.len = UDP_PKT_START + usedLen;

    udp_sendto(dst, 5683, ntohs(dport), &coapOutputBuffer);
}

void init_coaprouter() {
    udp_register(5683, NULL, coaprouter_udp_handler);
    // set up strlens ahead of time.
    uint8_t numendpoints = (sizeof(coap_endpoints)/sizeof(coap_endpoint));
    for (uint8_t i = 0; i < numendpoints; i++) {
       coap_endpoints[i].strlen = strlen_P(coap_endpoints[i].path);
    }
}

void coaprouter_periodic() {
    // Re-send any pending confirmable messages.
}