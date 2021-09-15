#include <avr/io.h>
#include <util/delay.h>
#include "nicreg.h"
#include "dbgserial.h"
#define nop()   __asm volatile ("nop")

#define ADDR_DDR PORTA.DIR
#define ADDR_MASK ((1<<5) | (1<<6) | (1<<7))
#define ADDR_PORT PORTA.OUT
#define CNTRL_PORT PORTA.OUT
#define CNTRL_READ_PIN (1<<6)
#define CNTRL_WRITE_PIN (1<<5)
#define CNTRL_CS_PIN (1<<7)
#define DATA_DDR PORTC.DIR
#define DATA_PORT PORTC.OUT
#define DATA_IN PORTC.IN

#define RST_DDR PORTB.DIR
#define RST_PORT PORTB.OUT
#define RST_PIN (1<<0)

//static unsigned char nextPage;                          // page pointer to next Rx packet
//static unsigned int currentRetreiveAddress;     // DMA address for read Rx packet location

static void port_setup() {
	// Setup our ports...
	CNTRL_PORT |= CNTRL_READ_PIN|CNTRL_WRITE_PIN;
	ADDR_DDR = 0xFF; // entire port is used as outputs.

	// Set data pins to pull-up mode (does not affect output);
	PORTC.PIN0CTRL |= PORT_OPC_PULLDOWN_gc;
	PORTC.PIN1CTRL |= PORT_OPC_PULLDOWN_gc;
	PORTC.PIN2CTRL |= PORT_OPC_PULLDOWN_gc;
	PORTC.PIN3CTRL |= PORT_OPC_PULLDOWN_gc;
	PORTC.PIN4CTRL |= PORT_OPC_PULLDOWN_gc;
	PORTC.PIN5CTRL |= PORT_OPC_PULLDOWN_gc;
	PORTC.PIN6CTRL |= PORT_OPC_PULLDOWN_gc;
	PORTC.PIN7CTRL |= PORT_OPC_PULLDOWN_gc;
	DATA_DDR = 0x00;
	
	RST_DDR |= RST_PIN;
}

void ax_write(uint8_t address, uint8_t data) {
	printf("write %x to %x\n", data, address);
	// Address setup
	ADDR_PORT = address | (ADDR_PORT & ADDR_MASK);	
	// Data on output.
	
	DATA_DDR = 0xFF;
	DATA_PORT = data;

	// Clock the write pin.
	CNTRL_PORT &= ~CNTRL_WRITE_PIN;
	nop(); // Technically this can be 1.25 instructions long...
	nop();
	CNTRL_PORT |= CNTRL_WRITE_PIN;

	// Data bus back as input.
	DATA_DDR = 0x00;
}

uint8_t ax_read(uint8_t address) {
	ADDR_PORT = address | (ADDR_PORT & ADDR_MASK);
	
	CNTRL_PORT &= ~CNTRL_READ_PIN;
	nop(); // 35 ns = 1.25 instructions...
	nop();
	uint8_t data = DATA_IN;
	CNTRL_PORT |= CNTRL_READ_PIN;
	printf("read %x from %x\n", data, address);
	return data;
}

void print_reg_state() {
	printf("CR: %x, ", ax_read(CR));
	printf("ISR: %x, ", ax_read(ISR));
	printf("RSR: %x\n", ax_read(RSR));	
}

void nic_init() {
	port_setup();
	DBGprintf("setup nic");
	_delay_ms(200);
	// Hard reset....
	RST_PORT &= ~RST_PIN;
	_delay_ms(200);
	RST_PORT |= RST_PIN;

	// Soft reset...
	_delay_ms(100);
	// Wait for PHY...
	ax_write(RSTPORT, ax_read(RSTPORT));
	while(ax_read(TR) & RST_B) {
		DBGprintf("wait awake\n");
	}
	while((ax_read(ISR) & RST_ISR) == 0) {
		DBGprintf("wait awake2\n");
		_delay_ms(1000);
	}
        ax_write(CR,(RD2|START));
        ax_write(ISR,0xFF);
	
	DBGprintf("awake!\n");
	
	while(1) {
		print_reg_state();
		_delay_ms(1000);
	}
}
