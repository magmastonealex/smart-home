#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "serial.h"

int main() {
    init_serial();
    //Setup i2c
    while(1) {
        _delay_ms(100);
	// Poll uno8
        printf("Values: %d %d %d\n", 100, 100, 100);
    }
}
