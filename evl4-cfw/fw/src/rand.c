#include "rand.h"

#include <avr/io.h>
#include <util/delay.h>
#include "dbgserial.h"

uint16_t random_get() {
    ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;
    ADCA.CTRLA = ADC_ENABLE_bm;
	ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc;
	ADCA.REFCTRL = ADC_REFSEL_INTVCC_gc | ADC_TEMPREF_bm;
    
    ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_INTERNAL_gc;
    ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;

    CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_IO_gc;

    for (uint8_t i = 0; i < 50; i++) {

        ADCA.CTRLA |= ADC_CH0START_bm;
        while((ADCA.INTFLAGS & ADC_CH0IF_bm) == 0); // Wait for conversion.
        uint8_t reading1 = ADCA.CH0RESL;

        ADCA.INTFLAGS = 0x01;
        ADCA.CTRLA &= ~(ADC_CH0START_bm);

        ADCA.CTRLA |= ADC_CH0START_bm;
        while((ADCA.INTFLAGS & ADC_CH0IF_bm) == 0); // Wait for conversion.
        uint8_t reading2 = ADCA.CH0RESL;

        CRC.DATAIN = ((reading1 & 0x0F) | ((reading2 & 0x0F)<<8));

        ADCA.INTFLAGS = 0x01;
        ADCA.CTRLA &= ~(ADC_CH0START_bm);

        _delay_ms(1);
    }

    //DBGprintf("checksum: %x %x %x %x\n", CRC.CHECKSUM0, CRC.CHECKSUM1, CRC.CHECKSUM2, CRC.CHECKSUM3);
    return ((uint16_t)(CRC.CHECKSUM0 ^ CRC.CHECKSUM1) << 8) | (uint16_t)(CRC.CHECKSUM2 ^ CRC.CHECKSUM3);

}