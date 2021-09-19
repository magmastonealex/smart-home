#define F_CPU 32000000

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "netstack.h"
#include "wdt.h"

#include "dbgserial.h"
#include "nic.h"
#include "timer.h"
#include "twi.h"

#include "alarm.h"

#include "coaprouter.h"


int main() {
	PORTD.DIR |= (1<<1) | (1<<0);
	PORTD.OUT |= (1<<0);
	PORTB.DIR |= (1<<0);
	// always enable the NIC - netstack will reset it when ready. The chip gets very hot if you leave it in reset for more than a few seconds.
	// Not the first weird thing about it.
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
	if (RST.STATUS & RST_WDRF_bm) {
		DBGprintf("WARN: WDT reset\n");
	} else if (RST.STATUS & RST_SRF_bm) {
		DBGprintf("WARN: SW reset\n");
	} else if (RST.STATUS & RST_PDIRF_bm) {
		DBGprintf("WARN: PDI reset\n");
	}
	netstack_init();
	sei();
	PMIC.CTRL |= PMIC_LOLVLEN_bm;

	init_timers();
	init_twi();
	init_coaprouter();

	init_monitoring();
	

	//PORTD.OUT |= (1<<3);
	uint8_t coapinterval = 5;
	uint8_t last_timer_tick = 0x00;
	while(1) {
		// netstack should be polled as often as possible to avoid dropping any packets.
		netstack_loop();


		if (last_timer_tick != (timer_get_ticks() & 0x00FF)) {
			last_timer_tick = (uint8_t)(timer_get_ticks() & 0x00FF);
			// monitoring_periodic wants 10ms ticks.
			monitoring_periodic();
			// 10ms tick, divided by 5 for coap...
			coapinterval--;
			if (coapinterval == 0) {
				coapinterval = 5;
				coaprouter_periodic();
			}
		}
	}
}
