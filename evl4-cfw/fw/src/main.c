#define F_CPU 32000000

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "netstack.h"

#include "dbgserial.h"
#include "nic.h"
#include "timer.h"

#include "app.h"


int main() {
	PORTD.DIR |= (1<<1) | (1<<0);
	PORTD.OUT |= (1<<0);
	PORTB.DIR |= (1<<0);
	PORTB.OUT |= (1<<0);
	// Switch to 32MHz operation.
	// Enable 32Mhz & 32.768KHz clocks.
	OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;
	// Wait for 32Mhz and 23Khz clocks to stabilize...
	while(!((OSC.STATUS & OSC_RC32MRDY_bm) && (OSC.STATUS & OSC_RC32KRDY_bm)));
	// Enable DFLL to calibrate 32Mhz clock.
	DFLLRC32M.CTRL |= (1<<0);

	// And switch to 32MHz clock.
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;

	init_serial();
	DBGprintf("hello world\n");
	netstack_init();
	sei();
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	init_timers();

	init_app();

	

	//PORTD.OUT |= (1<<3);
	while(1) {
		//_delay_ms(50);
		//PORTD.OUT |= (1<<1);
		//_delay_ms(50);
		//PORTD.OUT &= ~(1<<1);
		//printf("Timer %d  RTC %d\n", timer_get_ticks(), rtc_get_ticks());
		netstack_loop();
	}
}
