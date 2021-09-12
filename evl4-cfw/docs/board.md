
Chips
===

- Main CPU is Atmel ATXMEGA64D4
- MAC/PHY is ASIX ax88796blf. Has a dedicated crystal.
- 24LC512 EEProm
- RT8284N provides power to the board.

I2C Bus
====

  - UNO-8 lives at 0x20, 0x21, 0x22 - protocol documented separately
  - UNO-4 likely at 0x10
  - cell modem peripheral at 0x30?
  - 24LC512 eeprom at 0x50.

Headers
====

Programming
---

On the far left side, there's a board interconnect. At 0.1", you can easily solder a header to it. This exposes the ATXMEGA's PDI interface. The pinout is, from top to bottom:

- gnd 
- N/C 
- VCC 
- reset/PDI clock
- PDI data


Expansion
---

This is the main I2C bus interface. Contains +12V, SDA/SCL, GND. Need to double check ordering.

From left to right, with key facing up:

12V, SDA, SCL, GND

H6
---

This seems to be a 12V serial interface, connected to USARTD0.
The pinout seems to be, from left to right:
- 12V TX
- GND
- GND
- 12V RX
- +12V.

RST
---

This pad seems to be connected to PD4. Might be a factory reset of some kind??

LEDs
===

The LEDs are mostly driven by the MCU, with LINK being from the MAC/PHY.

All are common-anode (active high from MCU) except LINK.

- KEYB - PD1
- OPER - PD0
- NET - PB3
- ONLINE - PB2
- LINK - Driven off of ASIX LK/ACT


Pin Connections
===

The ATXMega pins are connected as follows:

- PA0 - ASIX SA0
- PA1 - ASIX SA1
- PA2 - ASIX SA2
- PA3 - ASIX SA3
- PA4 - ASIX SA4
- PA5 - ASIX WRn
- PA6 - ASIX RDn
- PA7 - ASIX CSn
- PB0 - ASIX RSTn (pulled low - needs to be asserted high
- PB1 - ASIX IRQ
- PB2 - ONLINE LED
- PB3 - NET LED
- PC - entire port connected to data lines 0-7 on ASIX.
- PD0 - OPER LED
- PD1 - KEYB LED
- PD2 - USART TX 
- PD3 - USART RX
- PD4 - RST header??
- PD5 - WP on EEPROM
- PD6 - N/C
- PD7 - N/C
- PE0/PE1-i2c
- PE2 - I/O pin GRN?
- PE3 - I/O pin YLW?
- PR1 - crystal not populated
- PR0 - crystal not populated.


Re-ordered for the ASIX MAC/PHY:

- Data 0:7 - PC
- SA0 - PA0
- SA1 - PA1
- SA2 - PA2
- SA3 - PA3
- SA4 - PA4
- WRn - PA5
- RDn - PA6
- CSn - PA7
- RSTn - PB0

Some notes on the ASIX:

- RST is pulled low. Needs to be asserted high by MCU to turn on.
- Operating in 8 bit mode.
- Using external dedicated oscillator

