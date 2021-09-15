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

static uint8_t nextPage;                          // page pointer to next Rx packet
static uint16_t currentRetreiveAddress;     // DMA address for read Rx packet location

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
	//printf("write %x to %x\n", data, address);
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
	//printf("read %x from %x\n", data, address);
	return data;
}

#define set_mdc		ax_write(MEMR,ax_read(MEMR)|0x01);
#define clr_mdc		ax_write(MEMR,ax_read(MEMR)&0xFE);

#define mii_clk		set_mdc; clr_mdc;				  
					
#define set_mdir	ax_write(MEMR,ax_read(MEMR)|0x02);
#define clr_mdir	ax_write(MEMR,ax_read(MEMR)&0xFD);
					
#define set_mdo		ax_write(MEMR,ax_read(MEMR)|0x08)
#define clr_mdo		ax_write(MEMR,ax_read(MEMR)&0xF7)

#define mii_write	clr_mdo; mii_clk;	\
					set_mdo; mii_clk;	\
					clr_mdo; mii_clk;	\
					set_mdo; mii_clk;

#define mii_read	clr_mdo; mii_clk;	\
					set_mdo; mii_clk;	\
					set_mdo; mii_clk;	\
					clr_mdo; mii_clk;

#define mii_r_ta    mii_clk;			\

#define mii_w_ta    set_mdo; mii_clk;	\
					clr_mdo; mii_clk;
			
void ax_writeMii(uint8_t phyad, uint8_t regad, uint8_t mii_data) {
	uint8_t mask8;
	uint16_t  i,mask16;

	mii_write;
 
	mask8 = 0x10;
	for(i=0;i<5;++i)
	{
  	   	if(mask8 & phyad)
			set_mdo;
		else
			clr_mdo;
		mii_clk;
		mask8 >>= 1;	 
	}   
	mask8 = 0x10;
	for(i=0;i<5;++i)
	{
  		if(mask8 & regad)
			set_mdo;
		else
			clr_mdo;
		mii_clk;
		mask8 >>= 1;	 
	}    					
	mii_w_ta;
 
	mask16 = 0x8000;
	for(i=0;i<16;++i)
	{
		if(mask16 & mii_data)
			set_mdo;
		else
			clr_mdo;
		mii_clk;	 
		mask16 >>= 1;	 
	}   			
}

uint16_t ax_readMii(uint8_t phyad, uint8_t regad) {
	uint8_t mask8,i;
	uint16_t  mask16,result16;
 
	mii_read;

	mask8 = 0x10;
	for(i=0;i<5;++i)
	{
		if(mask8 & phyad)
			set_mdo;
		else
			clr_mdo;
		mii_clk;	 
		mask8 >>= 1;
	}
	mask8 = 0x10;
	for(i=0;i<5;++i)
	{
		if(mask8 & regad)
			set_mdo;
		else
			clr_mdo;
		mii_clk;
		mask8 >>= 1;
	}
   			
	mii_r_ta;
 
	mask16 = 0x8000;
	result16 = 0x0000;
	for(i=0;i<16;++i)
	{
		mii_clk;
		if(ax_read(MEMR) & 0x04)
		{
			result16 |= mask16;
		}
		else
		{
			nop();
			break;
		}
		mask16 >>= 1;
	}
	return result16;
}
void ax_receiveOverflowRecover(void)
{
	DBGprintf("Receive overflow. Recovering...\n");
	// check if we were transmitting something
	uint8_t cmdReg = ax_read(CR);
	// stop the interface
	ax_write(CR, (RD2|STOP));
	// wait for timeout
	_delay_ms(2);
	// clear remote byte count registers
	ax_write(RBCR0, 0x00);
	ax_write(RBCR1, 0x00);

	uint8_t resend=0;	
	// if we were transmitting something
	if(cmdReg & TXP)
	{
		// check if the transmit completed
		cmdReg = ax_read(ISR);
		if((cmdReg & PTX) || (cmdReg & TXE))
			resend = 0;		// transmit completed
	    else
			resend = 1;		// transmit was interrupted, must resend
	}
	// switch to loopback mode
	ax_write(TCR, LB0);
	// start the interface
	ax_write(CR, (RD2|START));
	// set boundary
	ax_write(BNRY, RXSTART_INIT);
	// go to page 1
	ax_write(CR, (PS0|RD2|START));
	// set current page register
	ax_write(CPR, RXSTART_INIT+1);
	// go to page 0
	ax_write(CR, (RD2|START));
	// clear the overflow int
	ax_write(ISR, OVW);
	// switch to normal (non-loopback mode)
	ax_write(TCR, TCR_INIT);

	// if previous transmit was interrupted, then resend
	if(resend)
		ax_write(CR, (RD2|TXP|START));

	// recovery completed
}


void ax_processInterrupt(void){
	uint8_t intr = ax_read(ISR);
	
	// check for receive overflow
	if( intr & OVW )
		ax_receiveOverflowRecover();
}

void ax_beginPacketSend(uint16_t packetLength)
{
	uint16_t sendPacketLength;
	sendPacketLength = (packetLength>=0x3C ?(packetLength):0x3C);
	
	//start the NIC
	ax_write(CR,(RD2|START));
	
	// still transmitting a packet - wait for it to finish
	while( ax_read(CR) & TXP );

	//load beginning page for transmit buffer
	ax_write(TPSR,TXSTART_INIT);
	
	//set start address for remote DMA operation
	ax_write(RSAR0,0x00);
	ax_write(RSAR1,0x40);
	
	//clear the packet stored interrupt
	ax_write(ISR, PTX);

	//load data byte count for remote DMA
	ax_write(RBCR0, (uint8_t)(packetLength));
	ax_write(RBCR1, (uint8_t)(packetLength>>8));

	ax_write(TBCR0, (uint8_t)(sendPacketLength));
	ax_write(TBCR1, (uint8_t)((sendPacketLength)>>8));
	
	//do remote write operation
	ax_write(CR,0x12);
}


void ax_sendPacketData(uint8_t * localBuffer, uint16_t length)
{
	uint16_t i;
	
	for(i=0;i<length;i++)
		ax_write(RDMAPORT, localBuffer[i]);
}


void ax_endPacketSend(void)
{
	//send the contents of the transmit buffer onto the network
	ax_write(CR,(RD2|TXP));
	
	// clear the remote DMA interrupt
	ax_write(ISR, RDC);
}


void nic_send(uint16_t len, uint8_t* packet)
{
	ax_beginPacketSend(len);
	ax_sendPacketData(packet, len);
	ax_endPacketSend();
}

uint16_t ax_beginPacketRetreive(void)
{
	uint8_t writePagePtr;
	uint8_t readPagePtr;
	uint8_t bnryPagePtr;
	uint8_t i;
	
	uint8_t pageheader[4];
	uint16_t rxlen;
	
	// check for and handle an overflow
	ax_processInterrupt();
	
	// read CURR from page 1
	ax_write(CR,(PS0|RD2|START));
	writePagePtr = ax_read(CURR);
	// read the boundary register from page 0
	ax_write(CR,(RD2|START));
	bnryPagePtr = ax_read(BNRY);

	// first packet is at page bnryPtr+1
	readPagePtr = bnryPagePtr+1;
	if(readPagePtr >= RXSTOP_INIT) readPagePtr = RXSTART_INIT;
	
	// return if there is no packet in the buffer
	if( readPagePtr == writePagePtr )
	{
		return 0;
	}
	
	// clear the packet received interrupt flag
	ax_write(ISR, PRX);
	
	// if the boundary pointer is invalid,
	// reset the contents of the buffer and exit
	if( (bnryPagePtr < RXSTART_INIT) || (bnryPagePtr >= RXSTOP_INIT) )
	{
		ax_write(BNRY, RXSTART_INIT);
		ax_write(CR, (PS0|RD2|START));
		ax_write(CURR, RXSTART_INIT+1);
		ax_write(CR, (RD2|START));
		
//		rprintf("B");
		return 0;
	}

	// initiate DMA to transfer the RTL8019 packet header
	ax_write(RBCR0, 4);
	ax_write(RBCR1, 0);
	ax_write(RSAR0, 0);
	ax_write(RSAR1, readPagePtr);
	ax_write(CR, (RD0|START));
	for(i=0;i<4;i++)
		pageheader[i] = ax_read(RDMAPORT);

	// end the DMA operation
    ax_write(CR, (RD2|START));
    for(i = 0; i <= 20; i++)
        if(ax_read(ISR) & RDC)
            break;
    ax_write(ISR, RDC);
	
	rxlen = (pageheader[PKTHEADER_PKTLENH]<<8) + pageheader[PKTHEADER_PKTLENL];
	nextPage = pageheader[PKTHEADER_NEXTPAGE];
	
	currentRetreiveAddress = (readPagePtr<<8) + 4;
	
	// if the NextPage pointer is invalid, the packet is not ready yet - exit
	if( (nextPage >= RXSTOP_INIT) || (nextPage < RXSTART_INIT) )
	{
//		rprintf("N");
//		rprintfu08(nextPage);
		return 0;
	}

    return rxlen-4;
}


void ax_retreivePacketData(uint8_t * localBuffer, uint16_t length)
{
	uint16_t i;
	
	// initiate DMA to transfer the data
	ax_write(RBCR0, (uint8_t)length);
	ax_write(RBCR1, (uint8_t)(length>>8));
	ax_write(RSAR0, (uint8_t)currentRetreiveAddress);
	ax_write(RSAR1, (uint8_t)(currentRetreiveAddress>>8));
	ax_write(CR, (RD0|START));
	for(i=0;i<length;i++)
		localBuffer[i] = ax_read(RDMAPORT);
	// end the DMA operation
    ax_write(CR, (RD2|START));
    for(i = 0; i <= 20; i++)
        if(ax_read(ISR) & RDC)
            break;
    ax_write(ISR, RDC);
    
	currentRetreiveAddress += length;
	if( currentRetreiveAddress >= 0x6000 )
    	currentRetreiveAddress -= (0x6000-0x4600) ;
}

void ax_endPacketRetreive(void)
{
	uint8_t bnryPagePtr;

	// end the DMA operation
    ax_write(CR, (RD2|START));
    for(uint8_t i = 0; i <= 20; i++)
        if(ax_read(ISR) & RDC)
            break;
    ax_write(ISR, RDC);

	// set the boundary register to point
	// to the start of the next packet-1
        bnryPagePtr = nextPage-1;
	if(bnryPagePtr < RXSTART_INIT) bnryPagePtr = RXSTOP_INIT-1;

	ax_write(BNRY, bnryPagePtr);
}

uint16_t nic_poll(uint16_t maxlen, uint8_t* packet) {
	uint16_t packetLength;
	
	packetLength = ax_beginPacketRetreive();

	// if there's no packet or an error - exit without ending the operation
	if( !packetLength )
		return 0;

	// drop anything too big for the buffer
	if( packetLength > maxlen )
	{
		ax_endPacketRetreive();
		return 0;
	}
	
	// copy the packet data into the uIP packet buffer
	ax_retreivePacketData( packet, packetLength );
	ax_endPacketRetreive();
		
	return packetLength;
}

uint8_t PACKETBUFFER[500];
uint8_t regcount = 0;
void print_reg_state() {
	uint8_t sendpacket[60] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x00, 0x1C, 0x2A, 0x03, 0x2F, 0xF0,
0x08, 0x06,
0x00, 0x01,
0x08, 0x00,
0x06, 0x04,
0x00, 0x01,
0x00, 0x1C, 0x2A, 0x03, 0x2F, 0xF0,
0x0A, 0x66, 0x28, 0xFD,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x0A, 0x66, 0x28, 0x8E,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00,};
	DBGprintf("CR: %x, ", ax_read(CR));
	DBGprintf("ISR: %x, ", ax_read(ISR));
	DBGprintf("RSR: %x ", ax_read(RSR));
	DBGprintf("BNRY: %x\n", ax_read(BNRY));	
	DBGprintf("\n");
	uint16_t res = nic_poll(500, PACKETBUFFER);
	if(res > 0) {
			
		DBGprintf("Packet? %d\n", res);
		DBGprintf("dest: %x:%x:%x:%x:%x:%x ", PACKETBUFFER[0], PACKETBUFFER[1], PACKETBUFFER[2], PACKETBUFFER[3], PACKETBUFFER[4], PACKETBUFFER[5]);
		DBGprintf("src: %x:%x:%x:%x:%x:%x ", PACKETBUFFER[6], PACKETBUFFER[7], PACKETBUFFER[8], PACKETBUFFER[9], PACKETBUFFER[10], PACKETBUFFER[11]);
		DBGprintf("ethertype: %02x%02x\n", PACKETBUFFER[12], PACKETBUFFER[13]);
	}
	if(regcount > 5) {
		DBGprintf("Sending packet....\n");
		nic_send(60, sendpacket);	
		DBGprintf("sent\n");
		regcount = 0;
	}
	regcount++;
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
		_delay_ms(10);
	}
	while((ax_read(ISR) & RST_ISR) == 0) {
		DBGprintf("wait awake2\n");
		_delay_ms(10);
	}

	//ax_writeMii(0x10,0x00,0x0800);
	//_delay_ms(255);
	//ax_writeMii(0x10,0x00,0x1200);

	ax_write(CR,(RD2|STOP));		// stop the NIC, abort DMA, page 0
	_delay_ms(5);		// make sure nothing is coming in or going out
	ax_write(DCR,DCR_INIT); //zero data registers
	ax_write(RBCR0,0x00);
	ax_write(RBCR1,0x00);
	ax_write(IMR,0x00); // no interrupts
	ax_write(ISR,0xFF); // Clear any interrupts in progress
	ax_write(RCR,0x20); // make sure nothing tries to save to memory (monitor mode)
	ax_write(BNRY,RXSTART_INIT); // Setup receive start locations & boundary address.
	ax_write(PSTART,RXSTART_INIT);
	ax_write(PSTOP,RXSTOP_INIT);
	
	// switch to page 1
	ax_write(CR,(PS0|RD2|STOP));
	// write mac address
	ax_write(PAR0+0, 0x00);
	ax_write(PAR0+1, 0x1C);
	ax_write(PAR0+2, 0x2A);
	ax_write(PAR0+3, 0x03);
	ax_write(PAR0+4, 0x2F);
	ax_write(PAR0+5, 0xF0);
	// set start point
	ax_write(CURR,RXSTART_INIT+1);

	ax_write(CR,(RD2|START));
	ax_write(RCR,RCR_INIT); // Enable saving pacckets, and broadcast packets.
 
	ax_write(TPSR,TXSTART_INIT); // Set up TX buffer position.

	ax_write(CR,(RD2|STOP));
	ax_write(DCR,DCR_INIT);
	ax_write(CR,(RD2|START)); // Start the PHY for realzies!
	ax_write(ISR,0xFF);
	ax_write(IMR,0x10); // only overwrite interrupt
	

	
	DBGprintf("awake!\n");
	
	while(1) {
		print_reg_state();
		_delay_ms(1000);
	}
}


