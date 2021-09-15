#pragma once
#include <stdint.h>

void nic_init();
void nic_send(uint16_t len, uint8_t* packet);
uint16_t nic_poll(uint16_t maxlen, uint8_t* packet);
