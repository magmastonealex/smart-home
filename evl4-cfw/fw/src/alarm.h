#pragma once
#include "coap.h"
// here it is, at long last.
// The app that consumes all the hardware abstraction layers to produce sensor state data.

void cb_getcurrentvalues(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf);
void cb_getexpectedvalues(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf);
void cb_setexpectedvalues(void* data, coap_pkt* req, coap_pkt *res, sk_buff *buf);

void init_monitoring();
void monitoring_periodic();