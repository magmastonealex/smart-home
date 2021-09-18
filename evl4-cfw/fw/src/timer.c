#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint16_t ticker = 0;

ISR(TCC0_OVF_vect) {
    //PORTD.OUTTGL = (1<<1);
    ticker++;
}

void init_timers() {
    // Initialize TCC0 /1024,
    // running at 31250Hz.
    // We want to increment a counter every 10ms, so we'll set top...
    // 1/((1/(31250Hz))/0.010s) = x
    // x = 312.5 = 313.
    //
    // 3125 if 100ms.
    TCC0.PER = 313;
    TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc; // Set up over/underflow interrupt.
    TCC0.INTFLAGS = 0x01;
    TCC0.CTRLA = TC_CLKSEL_DIV1024_gc;

    //Initialize RTC, running at 1Hz exactly
    RTC.CTRL = RTC_PRESCALER_DIV1024_gc;
    CLK.RTCCTRL |= (1<<0) | (1<<2); // enable RTC, clocked at 1.024Khz


}

// Get a 10ms ticker value.
// This will roll over every 10 minutes -
uint16_t timer_get_ticks() {
    return ticker;
}

uint16_t rtc_get_ticks() {
    return RTC.CNT;
}
