#pragma once
#include <stdint.h>
// over-simplified EEPROM access driver.


#define EEPROM_SENSOR_NORMAL_START 0x00

uint8_t eeprom_read(uint8_t addr);
void eeprom_write(uint8_t addr, uint8_t data);