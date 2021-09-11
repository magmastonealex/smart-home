#include <avr/io.h>
#include <stdio.h>
static int uart_putchar(char c, FILE *stream);

static FILE serstdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

// initializes serial at 9600 baud 
void init_serial() {
    UBRR0=51;
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);

    stdout = &serstdout;
}

static int uart_putchar(char c, FILE *stream)
{
    if (c == '\n')
        uart_putchar('\r', stream);
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    return 0;
}
