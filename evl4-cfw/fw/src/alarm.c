#include "alarm.h"
#include "dbgserial.h"
#include "twi.h"
#include "coap.h"
#include "coaprouter.h"
#include <string.h>

static uint8_t sensorvalues_raw[8] = {0};
static uint8_t sensorvalues_processed[8] = {0}; // TODO: This could easily be a bitmask rather than array.
static uint8_t expectvalues[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t sensorvalues_dirty = 0; // Do we need to re-calculate sensorvalues_processed?

static uint8_t twiResp[10] = {0};
static twi_req twiReq;

// CoAP endpoint.
void cb_getcurrentvalues(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf) {
    res->hdr->code_class = 2;
    res->hdr->code_detail = 5; // 2.05 Content.

    coapScratch[0] = 42; // content-type - octet-stream.
    res->options[0].option_number = COAP_OPTION_CONTENT_FORMAT;
    res->options[0].option_value.len = 1;
    res->options[0].option_value.p = coapScratch;

    res->data.len = 8;
    res->data.p = sensorvalues_raw;
}

// CoAP endpoint.
void cb_getexpectedvalues(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf) {
    res->hdr->code_class = 2;
    res->hdr->code_detail = 5; // 2.05 Content.

    coapScratch[0] = 42; // content-type - octet-stream.
    res->options[0].option_number = COAP_OPTION_CONTENT_FORMAT;
    res->options[0].option_value.len = 1;
    res->options[0].option_value.p = coapScratch;

    res->data.len = 8;
    res->data.p = expectvalues;
}

// CoAP endpoint.
void cb_setexpectedvalues(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf) {
    if (req->data.len != 8) {
        res->hdr->code_class = 4;
        res->hdr->code_detail = 0; // 4.00 - bad request.
        res->data.len = 0;
    } else {

        res->hdr->code_class = 2;
        res->hdr->code_detail = 4; // 4.00 - bad request.
        
        memcpy(expectvalues, req->data.p, 8);
        sensorvalues_dirty = 1;

        coapScratch[0] = 42; // content-type - octet-stream.
        res->options[0].option_number = COAP_OPTION_CONTENT_FORMAT;
        res->options[0].option_value.len = 1;
        res->options[0].option_value.p = coapScratch;
        res->data.len = 8;
        res->data.p = expectvalues;

        // TODO: Commit the changed values to EEPROM.
    }

    
}

#define POLL_INTERVAL_10MS 1
static uint8_t readcounter = 0;
static uint8_t processedfirst = 0;
void init_monitoring() {
    // TODO: Read in expectValues from EEPROM.
    readcounter = 0;
    processedfirst = 0;
    twiReq.data = twiResp;
}

// Called every ~10ms.
void monitoring_periodic() {
    if (readcounter == 0) {
        readcounter = POLL_INTERVAL_10MS;
        

        create_adc_req(0x20, &twiReq);
        make_req(&twiReq);
    } else {
        readcounter--;
    }

    if (twiReq.fulfilled) {
        if (twiReq.success != 0) {
            DBGprintf("req fulfilled with err %u!\n", twiReq.success);
        } else {
            memcpy(sensorvalues_raw, twiReq.data, 8);
            sensorvalues_dirty = 1;
            // reset req so we don't process it again.
            twiReq.fulfilled = 0;
        }
    }

    if (sensorvalues_dirty) {
        // Look at sensorvalues_raw compared to expected.
        // If expected != raw +-5, set processed to 0xFF. 
        // If the value _changed_, report the sensor value to CoAP.
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t hicheck = expectvalues[i] > 0xFA ? 0xFA : expectvalues[i];
            uint8_t lowcheck = expectvalues[i] < 0x05 ? 0x05 : expectvalues[i];
            
            if ((sensorvalues_raw[i] > (hicheck + 5)) | (sensorvalues_raw[i] < (lowcheck - 5))) {
                // Sensor is tripped - it's out of the range of expected values.
                if (sensorvalues_processed[i] != 0xFF) {
                    // currently OFF - mark ON.
                    sensorvalues_processed[i] = 0xFF;
                    coap_update_sensor(i, 0xFF);
                }
            } else {
                // Sensor is _not_ tripped - in the range of expected values.
                if (sensorvalues_processed[i] != 0x00) {
                    // currently ON - mark OFF.
                    sensorvalues_processed[i] = 0x00;
                    coap_update_sensor(i, 0x00);
                }
            }
        }

        if (!processedfirst) {
            processedfirst = 1;
            coap_mark_ready(); // sensor values are correct now.
        }
    }
}