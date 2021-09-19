#include <avr/io.h>
#include "dbgserial.h"
#include "twi.h"
#include "errno.h"
#include <avr/interrupt.h>
#include <util/atomic.h>

// A rough around the edges (sensing a theme yet?)
// TWI driver for atxmega.
// Tailored to the UNO expansion boards from Envisalink,
// which use a command-response protocol.

#define TWI_STATE_IDLE 0 // Doing nothing, bus released.
#define TWI_STATE_CMD_START_ADDR 1 // START has been sent, waiting for confirmation.
#define TWI_STATE_CMD_DATA 3 // Command has been sent, waiting for ACK/NAK
#define TWI_STATE_DATA_START_ADDR 4 // START has been sent to receive data, waiting for confirmation.
#define TWI_STATE_DATA_DATA 6 // Waiting for data byte to arrive.

static uint8_t state = 0;
static volatile twi_req *activeReq = NULL;

ISR(TWIE_TWIM_vect) {
    if (state == TWI_STATE_IDLE) {
		// Nothing to do!
	}
	else if (state == TWI_STATE_CMD_START_ADDR) {
        // On startup, TWIE.MASTER.ADDR is set to the right value,
        // which does a START and sends the address.
        // We now look for bus errors...
        if ((TWIE.MASTER.STATUS & (TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm)) != 0) {
            activeReq->success = EUNKNOWN;
            activeReq->fulfilled = 1;
            activeReq = NULL;
            state = TWI_STATE_IDLE;
            return;
        } else if ((TWIE.MASTER.STATUS & TWI_MASTER_RXACK_bm) == TWI_MASTER_RXACK_bm) { // and if the slave naked the request.
            // NAK from slave.
			activeReq->success = EUNAVAIL;
            activeReq->fulfilled = 1;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
            TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
			return;
		}
        TWIE.MASTER.DATA = activeReq->cmd;
		state = TWI_STATE_CMD_DATA;
	}
	else if (state == TWI_STATE_CMD_DATA) {
		// Did the slave ack our data byte?
		// We now look for bus errors...
        if ((TWIE.MASTER.STATUS & (TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm)) != 0) {
            activeReq->success = EUNKNOWN;
            activeReq->fulfilled = 1;
            activeReq = NULL;
            state = TWI_STATE_IDLE;
            //TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
            return;
        } else if ((TWIE.MASTER.STATUS & TWI_MASTER_RXACK_bm) == TWI_MASTER_RXACK_bm) { // and if the slave naked the request.
            // NAK from slave.
			activeReq->success = EUNAVAIL;
            activeReq->fulfilled = 1;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
            TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
			return;
		}

		if (activeReq->dataLength == 0) {
			//Done!
            TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
			state = TWI_STATE_IDLE;
			activeReq->success = 0;
            activeReq->fulfilled = 1;
			activeReq = NULL;
			return;
		} else {
			// Send a repeated start, with intent to read.
            TWIE.MASTER.ADDR = activeReq->addr | (1<<0);
			state = TWI_STATE_DATA_DATA;
		}
	} else if (state == TWI_STATE_DATA_DATA) {
		// Got control of bus?
		// We now look for bus errors...
        if ((TWIE.MASTER.STATUS & (TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm)) != 0) {
            activeReq->success = EUNKNOWN;
            activeReq->fulfilled = 1;
            activeReq = NULL;
            state = TWI_STATE_IDLE;
            //TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
            return;
        } else if ((TWIE.MASTER.STATUS & (TWI_MASTER_RXACK_bm | TWI_MASTER_WIF_bm)) == (TWI_MASTER_RXACK_bm | TWI_MASTER_WIF_bm)) { // If we got a NAK during the start, this will be hit.
            // NAK from slave.
			activeReq->success = EUNAVAIL;
            activeReq->fulfilled = 1;
			activeReq = NULL;
			state = TWI_STATE_IDLE;
            TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
			return;
		}

        activeReq->data[activeReq->dataRead++] = TWIE.MASTER.DATA;
        // do we have any more data to read, or is this the last byte?
        if(activeReq->dataRead == activeReq->dataLength) {
			// We've read our final byte. NAK & send STOP.
            // Make sure we'll send a STOP...
			TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
			state = TWI_STATE_IDLE;
			activeReq->success = 0;
            activeReq->fulfilled = 1;
			activeReq = NULL;
			return;
		}
        else {
            // still got data to read!
            TWIE.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
        }
	}
	else {
		// Something went very wrong.
		state = TWI_STATE_IDLE;
		if (activeReq != NULL) {
			activeReq->success = EUNKNOWN;
            activeReq->fulfilled = 1;
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
		while((TWIE.MASTER.STATUS & TWI_MASTER_BUSSTATE_BUSY_gc) != TWI_MASTER_BUSSTATE_IDLE_gc);
		activeReq = req;
		state = TWI_STATE_CMD_START_ADDR;
        TWIE.MASTER.ADDR = activeReq->addr;
		//TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE);
	}
	return 0;
}

void init_twi() {

    // Set up for 400KHz operation.
    TWIE.MASTER.BAUD = 35;

    TWIE.MASTER.CTRLA = TWI_MASTER_INTLVL_LO_gc | TWI_MASTER_RIEN_bm  | TWI_MASTER_WIEN_bm | TWI_MASTER_ENABLE_bm;
    //TWIE.MASTER.CTRLB |= TWI_MASTER_SMEN_bm;
    
    // We're the only master - we can force the bus into IDLE state.
    TWIE.MASTER.STATUS |= TWI_MASTER_BUSSTATE_IDLE_gc;

}