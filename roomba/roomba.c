#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL
#include <util/delay.h>

#define BURST_ON        500      // Burst on duration (uSec)
#define BURST_OFF        7500    // Burst off duration (uSec)
#define BURST_COUNT      3        // How many IR bursts to send

//#define BURST_SPACE      132      // 1/31000 * 4096 (clock freq * WDT postscalar) (mSec)

ISR(WDT_vect) {
}

// Continously broadcasts the iRobot Roomba virtual wall signal.
// Hook up an IR LED with + on PA6 (or change that pin below).
// You should use a current-limiting resistor, or use a transistor to drive the LED
// with more power - generally fine because it's only on for fractions of a second!
void main(){
	
	MCUSR = 0;

	PRR |= ((1<<PRTIM0) | (1<<PRUSI) | (1<<PRADC));

	DDRA |= (1<<PA6);

	PORTA &= ~(1<<PA6);
	
	const uint16_t pwmval = F_CPU / 2000 / (38); \
	TCCR1A                = _BV(WGM11); \
	TCCR1B                = _BV(WGM13) | _BV(CS10); \
	ICR1                  = pwmval; \
	OCR1A                 = pwmval / 3; \

	sei();

	TCCR1A |= _BV(COM1A1);

	while(1){
		for(int i = 0; i < 3; i++) {
			TCCR1A |= _BV(COM1A1);

			_delay_us(BURST_ON);
			
			TCCR1A &= ~(_BV(COM1A1));

			PORTA &= ~(1<<PA6);

			_delay_us(BURST_OFF);
		}

		cli();
		WDTCSR = (1<<WDCE)|(1<<WDE);
		WDTCSR = (1<<WDE)|(1<<WDIE) | (1<<WDP0) | (1<<WDP1);
		sei();

  		sleep_mode();

		cli();
		WDTCSR = (1<<WDCE)|(1<<WDE);
		WDTCSR = 0;
		sei();
	}
}
