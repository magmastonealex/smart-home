#include "wdt.h"
#include <avr/io.h>


// Enable our watchdog timer at 1s.
// the alarm app kicks the watchdog every time a success response is received from the 
// i2c bus.
void watchdog_init() {
    CCP = CCP_IOREG_gc;
    WDT.CTRL = WDT_PER_512CLK_gc | WDT_ENABLE_bm | WDT_CEN_bm;
}

#define WDT_RESET()  __asm volatile ("wdr")

void watchdog_kick() {
    WDT_RESET();
}

void watchdog_sw_reset() {
    CCP = CCP_IOREG_gc;
    RST.CTRL = 0x01;
}