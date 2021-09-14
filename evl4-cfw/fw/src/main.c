#define F_CPU 32000000

#include <avr/io.h>
#include <util/delay.h>

int main() {
	PORTD.DIR |= (1<<1) | (1<<0);
	PORTD.OUT |= (1<<0);

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
	
	while(1) {
		_delay_ms(200);
		PORTD.OUT |= (1<<1);
		_delay_ms(200);
		PORTD.OUT &= ~(1<<1);
	}
}
