CFW for EVL-4 by Envisalink
---

After purchasing an EVL-4 "UNO" to get some convential alarm sensors into Home Assistant,
I found the default firmware lacking. The "TPI" interface seems broken and unable to report on zone state,
and trips the alarm on the unit regularly.

The hardware, however, seems quite decent.

The UNO-8 module is interesting, over something like Konnected.io, because it has support for
detecting EOL resistors, and two outputs for a siren or similar. They can also be easily daisy-chained.

The EVL-4 board has an ATXMega64D4, and an ASIX Ethernet PHY/MAC capable of 10/100 Ethernet. The XMEGA's PDI interface is exposed via an external connector.

The two boards communicate over a relatively simple I2C protocol (full docs coming eventually. Notes.txt has some details).
I could just hook it up to an esp8266 and call it a day, but I'd prefer hard-wired solutions for an alarm system.

Given the board's relatively simple layout & well-documented components, I'm going to try to write a custom firmware for it
which can report sensor states to an MQTT server. We'll see how far I get.

Things I'm working on:

- [x] Reverse engineering board layout (done! See notes.txt. Needs some better docs)
- [x] Reverse engineer UNO-8 protocol (Mostly done, see notes.txt. Need to better understand the two output channels.)
- [ ] Implement a test program to demonstrate the UNO-8 can work independently (in progress!)
- [ ] Bring up a basic firmware to test functionality of the EVL-4 (serial, LEDs)
- [ ] Add support for the Ethernet PHY/MAC via uIP
- [ ] Bring in the UNO-8 library developed as part of the test program
- [ ] Add support for MQTT via lwmqtt (after evaluating code size)
