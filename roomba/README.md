This is code & a few example schematics for Roomba virtual walls using an attiny84.

Virtual walls are IR transmitters that are able to block off areas to prevent a Roomba robot vacuum from entering rooms or areas.
They require very little code, just a PWM carrier wave, and on-off keying. Hard to do with 555 timer.

I usually use some heat-shrink to cover the LED and make a "tunnel" to help focus the "wall" down a single straight line. Otherwise, the roomba won't like driving near the LED at all. You can "diffuse" the LED any way you'd like and it may provide "force field" functionality.

This should work on very nearly any AVR microcontroller. atmega's are overkill unless you're doing something else with them (like running temp. sensor code or reporting battery over radio.) You may need to change some registers.

I've set these up in three major ways:

- IR LED driven straight off the microcontroller pin (not advisable...), running on 3v battery. This is really easy to "dead bug" solder. Short-ish range though - maybe only 3ft.
- IR LED driven off a transistor, running at 150mA or so. You should probably aim for 100mA, but I didn't have the right resistors. This is good for longer-rage - I've gotten it to work for several meters.
- Multiple IR LEDs in different directions, off a MOSFET. Aiming for 100mA, running off AA batteries at around 4.5V. This allowed for a single device to sit under a couch side-table and block off two directions at once.
