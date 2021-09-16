#pragma once
#include "netcommon.h"
#include <stdint.h>

void nic_write_mac(uint8_t *loc);
void nic_init();
void nic_send(sk_buff* packet);
uint16_t nic_poll(uint16_t maxlen, uint8_t* packet);
