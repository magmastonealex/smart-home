#include "coaprouter.h"
#include "udp.h"
#include "coap.h"
#include "dbgserial.h"
#include <string.h>
#include <avr/pgmspace.h>


// TODOs:
//  - plumb interval into main.
//

const char wellknownsdresp[] PROGMEM = "</rawvalues>;title=\"current raw adc values. Returns 8 bytes, one per channel.\",\n";
const char pubsubendpoint[] PROGMEM = "publish";


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

// changes to coap_sensors are detected (marked dirty/clean)
// and sent out ASAP. Retries if not confirmed immediately happen
// every 300ms.
typedef struct {
    uint8_t sensorId;
    uint8_t value;

    uint8_t dirty;
    uint8_t sendtimer;
    uint16_t ack;
} coap_sensor;

uint16_t sensoracks = 0;

#define REPORT_ALL_INTERVAL_50MS 60
#define SENSOR_RETRY_INTERVAL_50MS 6
// In units of 50ms, how long till next report-all gets triggered?
// Normally done every 3 seconds, also serves as our heartbeat.
// This report-all is sent non-reliably, given the frequency.
uint8_t sensorsReportAllTimer = 0;
uint8_t sensorsReady = 0;
coap_sensor allSensors[] = {
    {0},
    {1, 0, 0, 0, 0},
    {2, 0, 0, 0, 0}
};

void coap_update_sensor(uint8_t sensorid, uint8_t value) {
    if(allSensors[sensorid].value != value) {
        allSensors[sensorid].value = value;
        allSensors[sensorid].dirty = 1;
        allSensors[sensorid].sendtimer = 0;
        allSensors[sensorid].ack = sensoracks++;
    }
}

// Sensor values will not be reported until this is called. It will also mark all sensors as dirty.
void coap_mark_ready() {
    sensorsReady = 1;
    sensorsReportAllTimer = 0;
    uint8_t numsensors = sizeof(allSensors) / sizeof(coap_sensor);
    for (uint8_t i = 0; i < numsensors; i++)
    {
        allSensors[i].dirty = 1;
        allSensors[i].sendtimer = 2+i; // don't send all 8 at once...
        allSensors[i].ack = sensoracks++;
    }
}

void coap_sensor_resp_received(uint16_t ackid) {
    uint8_t numsensors = sizeof(allSensors) / sizeof(coap_sensor);
    for (uint8_t i = 0; i < numsensors; i++)
    {
        if (allSensors[i].ack == ackid) {
            allSensors[i].dirty = 0;
            break;
        }
    }
    
}

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
    
    if (inpkt.hdr->type == COAP_TYPE_ACK) {
        coap_sensor_resp_received(ntohs(inpkt.hdr->msgid_be));
    }

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


static void send_update(uint8_t type, uint8_t *upd, uint8_t update_len, uint16_t msgid) {
    memset((uint8_t*)&outpkt, 0, sizeof(coap_pkt));
    outpkt.hdr = &outpktHdr;

    outpkt.hdr->msgid_be = htons(msgid);
    outpkt.hdr->type = type;
    outpkt.hdr->ver = 1;
    
    outpkt.token.len = 0;
    outpkt.hdr->tkl = 0;

    outpkt.hdr->code_class = 0;
    outpkt.hdr->code_detail = 2;

    memcpy_P(coapScratch, pubsubendpoint, 7);
    outpkt.options[0].option_number = 11;
    outpkt.options[0].option_value.len = 7;
    outpkt.options[0].option_value.p = coapScratch;

    outpkt.data.len = update_len;
    outpkt.data.p = upd;

    coapOutputBuffer.len = NET_MTU;
    uint16_t usedLen = 0;
    uint8_t ret = coap_serialize(&outpkt, coapOutputBuffer.buff+UDP_PKT_START, coapOutputBuffer.len - UDP_PKT_START, &usedLen);
    if (ret != 0) {
        DBGprintf("coap serialize failed: %u", ret);
        return;
    }
    
    coapOutputBuffer.len = UDP_PKT_START + usedLen;

    udp_sendto(IPADDR_FROM_OCTETS(192, 168, 50, 1), 5683, 5683, &coapOutputBuffer);
}

uint8_t updateScratch[sizeof(allSensors) / sizeof(coap_sensor)];
// Should get called every ~50ms.
void coaprouter_periodic() {
    // If our allReport timer has ticked down, then send the state of all sensors.
    uint8_t numsensors = sizeof(allSensors) / sizeof(coap_sensor);
    if (sensorsReportAllTimer == 0) {
        for (uint8_t i = 0; i < numsensors; i++) {
            updateScratch[i] = allSensors[i].value;
        }
        send_update(COAP_TYPE_NON_CONFIRMABLE, updateScratch, numsensors, sensoracks++);
        sensorsReportAllTimer = REPORT_ALL_INTERVAL_50MS;
    }

    // Look for any dirty bits w/ zero timers, and send packets for those specifically.

    for (uint8_t i = 0; i < numsensors; i++) {
        if(allSensors[i].dirty) {
            if (allSensors[i].sendtimer == 0) {
                updateScratch[0] = allSensors[i].value;
                send_update(COAP_TYPE_CONFIRMABLE, updateScratch, 1, allSensors[i].ack);
                allSensors[i].sendtimer = SENSOR_RETRY_INTERVAL_50MS;
            } else {
                allSensors[i].sendtimer--;
            }
        }
    }
    
}