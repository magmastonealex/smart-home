#include "eeprom.h"
#include <avr/io.h>


#define NVM_EXEC()	__asm volatile ("push r30"      "\n\t"	\
			    "push r31"      "\n\t"	\
    			    "push r16"      "\n\t"	\
    			    "push r18"      "\n\t"	\
			    "ldi r30, 0xCB" "\n\t"	\
			    "ldi r31, 0x01" "\n\t"	\
			    "ldi r16, 0xD8" "\n\t"	\
			    "ldi r18, 0x01" "\n\t"	\
			    "out 0x34, r16" "\n\t"	\
			    "st Z, r18"	    "\n\t"	\
    			    "pop r18"       "\n\t"	\
			    "pop r16"       "\n\t"	\
			    "pop r31"       "\n\t"	\
			    "pop r30"       "\n\t"	\
			    )


static void await_nvm() {
    do {
		/* Block execution while waiting for the NVM to be ready. */
	} while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm);
}

uint8_t eeprom_read(uint8_t byteAddr) {
    await_nvm();

    /* Calculate address */
	uint16_t address = (uint16_t)(0*EEPROM_PAGE_SIZE)
	                            |(byteAddr & (EEPROM_PAGE_SIZE-1));

	/* Set address to read from. */
	NVM.ADDR0 = address & 0xFF;
	NVM.ADDR1 = (address >> 8) & 0x1F;
	NVM.ADDR2 = 0x00;

	/* Issue EEPROM Read command. */
	NVM.CMD = NVM_CMD_READ_EEPROM_gc;
	NVM_EXEC();

	uint8_t tmp =  NVM.DATA0;
    NVM.CMD = NVM_CMD_NO_OPERATION_gc;
    return tmp;
}

void eeprom_write(uint8_t byteAddr, uint8_t value) {
	/* Wait until NVM is not busy. */
	await_nvm();

	/* Flush EEPROM page buffer if necessary. */
	if ((NVM.STATUS & NVM_EELOAD_bm) != 0) {
		NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
		NVM_EXEC();
	}

    NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

	/* Calculate address */
	uint16_t address = (uint16_t)(0*EEPROM_PAGE_SIZE)
	                            |(byteAddr & (EEPROM_PAGE_SIZE-1));

	/* Set address to write to. */
	NVM.ADDR0 = address & 0xFF;
	NVM.ADDR1 = (address >> 8) & 0x1F;
	NVM.ADDR2 = 0x00;

	/* Load data to write, which triggers the loading of EEPROM page buffer. */
	NVM.DATA0 = value;

	/*  Issue EEPROM Atomic Write (Erase&Write) command. Load command, write
	 *  the protection signature and execute command.
	 */
	NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
	NVM_EXEC();

    NVM.CMD = NVM_CMD_NO_OPERATION_gc;
}