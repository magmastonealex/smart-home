#pragma once
#include <stdint.h>

// Describes a TWI request for UNO I/O expander.
typedef struct twi_req {
	uint8_t addr;
	uint8_t cmd;
	
	uint8_t dataLength; // if 0, no read will be performed.
	uint8_t dataRead;
	uint8_t *data; // resp should point to a place to put response data, if applicable.

	uint8_t fulfilled; // Will be set to 1 by ISR when transaction is complete
	uint8_t success; // Will be set to 0 if successful, or a errno value otherwise.
} twi_req;

#define ID_REQ_RESP_LENGTH 3
#define ADC_REQ_RESP_LENGTH 8


void init_twi();
uint8_t make_req(volatile twi_req *req);
void create_id_req(uint8_t addr, volatile twi_req *req);
void create_adc_req(uint8_t addr, volatile twi_req *req);