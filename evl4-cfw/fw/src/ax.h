#pragma once
#include <stdint.h>

void ax_init();
void ax_print_registers();

uint16_t ax_beginPacketRetreive(void);
void ax_retreivePacketData(uint8_t * localBuffer, uint16_t length);
void ax_endPacketRetreive(void);

void ax_beginPacketSend(uint16_t packetLength);
void ax_sendPacketData(uint8_t * localBuffer, uint16_t length);
void ax_endPacketSend(void);

void ax_processInterrupt(void);
void ax_receiveOverflowRecover(void);
