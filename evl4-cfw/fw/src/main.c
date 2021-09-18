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

#include "coaprouter.h"


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
	
	init_coaprouter();

	coap_mark_ready();	

	//PORTD.OUT |= (1<<3);
	uint8_t coapinterval = 5;
	uint8_t last_rtc_tick = 0x00;
	uint8_t last_timer_tick = 0x00;
	while(1) {
		netstack_loop();

		// don't care about absolute values, just care about changes in the low byte.
		if (last_rtc_tick != (rtc_get_ticks() & 0x00FF)) {
			last_rtc_tick = (uint8_t)(rtc_get_ticks() & 0x00FF);
			// 1 second tick. For now, we'll use this to change a sensor's state.
			coap_update_sensor(1, (uint8_t)(last_rtc_tick & 0x00FF));
		}

		if (last_timer_tick != (timer_get_ticks() & 0x00FF)) {
			last_timer_tick = (uint8_t)(timer_get_ticks() & 0x00FF);
			// 10ms tick.
			coapinterval--;
			if (coapinterval == 0) {
				coapinterval = 5;
				coaprouter_periodic();
			}
		}
	}
}
