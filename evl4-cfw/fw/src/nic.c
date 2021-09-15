#include <stdint.h>
#include "ax.h"


void nic_send(uint16_t len, uint8_t* packet)
{
	ax_beginPacketSend(len);
	ax_sendPacketData(packet, len);
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


