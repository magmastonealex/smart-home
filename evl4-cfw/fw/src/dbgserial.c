#include <avr/io.h>

#ifdef DEBUG
#include <stdio.h>
static int uart_putchar(char c, FILE *stream) {
    if (c == '\n')
        uart_putchar('\r', stream);
    // Wait for empty buffer...
    while(!(USARTD0.STATUS & USART_DREIF_bm));
    USARTD0.DATA = c;
    return 0;
}

static FILE serstdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
#endif
// initializes serial on PORTD at 38.4Kbaud. Assumes 32Mhz operation...
// TODO: make baud confiurable and depend on F_CPU.
void init_serial() {
	#ifdef DEBUG
    PORTD.DIR |= (1<<2);
    PORTD.OUT |= (1<<2);

	// 38.4K, @32Mhz, BSEL = 12, BSCALE=2, CLK2X=0.
	USARTD0.BAUDCTRLA=12;
	USARTD0.BAUDCTRLB=0b00100000;
	USARTD0.CTRLB |= USART_TXEN_bm;

    stdout = &serstdout;
	#endif
}



