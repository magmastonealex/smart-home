#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"
#include "twi.h"

volatile uint8_t counter=0;

ISR(TIMER1_OVF_vect){
	counter++;
}

int main() {
    init_serial();
    init_twi();

    cli();
    TCCR1B |= (1<<CS12);
    TIMSK1 |= (1<<TOIE1);
    sei();

    uint8_t twiData[10];
    volatile twi_req req = {0};
    req.data = twiData;
    create_id_req(0x20, &req); 
    uint8_t res = make_req(&req);
    if (res != 0) {
	while(1) {
	printf("Failed to make i2c request! %x\n", res);
	_delay_ms(5000);
	}
    }
    //Setup i2c
    while(1) {
	// Poll uno8
	//_delay_ms(1);
        //printf("Values: %d %x %x\n", counter, counter, TCNT1);
	if(req.fulfilled) {
		printf("status: %x, vals %x %x %x\n", req.success, req.data[0], req.data[1], req.data[2]);
		create_adc_req(0x20, &req);
		    res = make_req(&req);
		    if (res != 0) {
			while(1) {
			printf("Failed to make i2c request! %x\n", res);
			_delay_ms(5000);
			}
		    }

	}
    }
}
