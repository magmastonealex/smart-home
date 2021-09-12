#include <avr/io.h>
#include <util/twi.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "twi.h"
#include "errno.h"
/*
 Implements a state-machine based TWI interface, specific to talking to
 Envisalink UNO alarm system I/O expanders.
*/

#define TWI_STATE_IDLE 0 // Doing nothing, bus released.
#define TWI_STATE_CMD_START 1 // START has been sent, waiting for confirmation.
#define TWI_STATE_CMD_ADDR 2 // Slave address has been sent, waiting for ACK/NAK
#define TWI_STATE_CMD_DATA 3 // Command has been sent, waiting for ACK/NAK
#define TWI_STATE_DATA_START 4 // START has been sent to receive data, waiting for confirmation.
#define TWI_STATE_DATA_ADDR 5 // Slave address has been sent, waiting for ACK/NAK
#define TWI_STATE_DATA_DATA 6 // Waiting for data byte to arrive.

static uint8_t state = 0;
static volatile twi_req *activeReq = NULL;

ISR(TWI_vect) {
	if (state == TWI_STATE_IDLE) {
		// Nothing to do!
	}
	else if (state == TWI_STATE_CMD_START) {
	        if ((TWSR & 0xF8) != TW_START) {
			activeReq->fulfilled = 1;
			activeReq->success = EUNKNOWN;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
			return;
        	}
		// Send out our address in "write" mode.
		TWDR = activeReq->addr;
	        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE);
		state = TWI_STATE_CMD_ADDR;
	}
	else if (state == TWI_STATE_CMD_ADDR) {
		// is the slave available?
		if ((TWSR & 0xF8) != TW_MT_SLA_ACK) {
			activeReq->fulfilled = 1;
			activeReq->success = EUNAVAIL;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
			return;
		}

		// Write our command byte.
		TWDR = activeReq->cmd;
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE);
		state = TWI_STATE_CMD_DATA;
	}
	else if (state == TWI_STATE_CMD_DATA) {
		// Did the slave ack our request?
		if ((TWSR & 0xF8) != TW_MT_DATA_ACK) {
			activeReq->fulfilled = 1;
			activeReq->success = EINVAL;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
			return;
		}

		if (activeReq->dataLength == 0) {
			//Done!
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
			state = TWI_STATE_IDLE;
			activeReq->fulfilled = 0;
			activeReq->success = 0;
			activeReq = NULL;
			return;
		} else {
			// Send a repeated start.
			TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE);
			state = TWI_STATE_DATA_START;
		}
	} else if (state == TWI_STATE_DATA_START) {
		// Got control of bus?
		if ((TWSR & 0xF8) != TW_REP_START) {
			activeReq->fulfilled = 1;
			activeReq->success = EUNKNOWN;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
			return;
		}
		// Set up read from the slave
	        TWDR = activeReq->addr | (1<<0);
	        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE);
		state = TWI_STATE_DATA_ADDR;
	} else if (state == TWI_STATE_DATA_ADDR) {
		if ((TWSR & 0xF8) != TW_MR_SLA_ACK) {
			activeReq->fulfilled = 1;
			activeReq->success = EUNAVAIL;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
			return;
		}
		// Start reading data.
		// Only one byte? Send NAK, Else, ACK.
		if (activeReq->dataLength == 1) {
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE);
		}
		else {
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA) | (1<<TWIE);
		}
		state = TWI_STATE_DATA_DATA;
	}
	else if (state == TWI_STATE_DATA_DATA) {
		if ((TWSR & 0xF8) == TW_BUS_ERROR) {
			activeReq->fulfilled = 1;
			activeReq->success = EUNKNOWN;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
			return;
		}
		
		// Was this the last byte?
		activeReq->data[activeReq->dataRead++] = TWDR;
		if(activeReq->dataRead == activeReq->dataLength) {
			// We read everything. yay!
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
			state = TWI_STATE_IDLE;
			activeReq->fulfilled = 1;
			activeReq->success = 0;
			activeReq = NULL;
			return;
		}
		else if ((activeReq->dataRead+1) == activeReq->dataLength) { // Is the next byte the last byte?
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE);
		} else { // More data! 
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA) | (1<<TWIE);
		}
	}
	else {
		// Something went very wrong.
		state = TWI_STATE_IDLE;
		if (activeReq != NULL) {
			activeReq->fulfilled = 1;
			activeReq->success = EUNKNOWN;
		}
	}
}

// Populate a request for identification.
// Assumes resp points to a buffer of ID_REQ_RESP_LENGTH size.
void create_id_req(uint8_t addr, volatile twi_req *req) {
	req->addr = addr<<1;
	req->cmd = 0x00;
	req->dataLength = ID_REQ_RESP_LENGTH;
	req->dataRead = 0;
	req->fulfilled = 0;
	req->success = 0;
}

void create_adc_req(uint8_t addr, volatile twi_req *req) {
	req->addr = addr<<1;
	req->cmd = 0x10;
	req->dataLength = ADC_REQ_RESP_LENGTH;
	req->dataRead = 0;
	req->fulfilled = 0;
	req->success = 0;
}

uint8_t make_req(volatile twi_req *req) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (activeReq != NULL) {
			return EBUSY;
		}
		// Make sure the last transaction has finished (should take no more than a ms)
		while(TWCR & (1<<TWSTO));
		activeReq = req;
		state = TWI_STATE_CMD_START;
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE);
	}
	return 0;
}

// Assumes 8MHz CPU when setting up prescaler
void init_twi() {
	// Prescaler = 16
	// TWBR=2
	TWBR=2;
	TWSR|=(1<<TWPS1);
}

