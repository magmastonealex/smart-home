#include "nic.h"
#include <avr/pgmspace.h>
#include <stdint.h>
#include "ax.h"
#include "dbgserial.h"

const uint8_t MACADDR[6] PROGMEM = {0x00, 0x1C, 0x2A, 0x03, 0x2F, 0xF0};

void nic_write_mac(uint8_t *loc) {
	memcpy_P(loc, MACADDR, 6);
}

void nic_send(sk_buff* packet)
{
	ax_beginPacketSend(packet->len);
	ax_sendPacketData(packet->buff, packet->len);
	ax_endPacketSend();
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

void print_reg_state() {
	ax_print_registers();
}

void nic_init() {
	ax_init();
}


