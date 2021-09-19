Custom FW for EVL-4 by Envisalink
---

**Note**: This project is not endorsed in any way shape or form by Envisalink. It might make your board blow up, fry your alarm system, or permanently brick your router. I make no endorsement whatsoever as to the quality or robustness of this code, and you assume all risk from making use of this code.

**Note The Second**: This project was a fun way to spend a few evenings building an IP stack from scratch, and reverse engineering some hardware. The fact that I just wrote "building an IP stack from scratch" should imply this, but to make it abundantly clear: *I cannot in any way recommend you make use of this project*. It's written for my super narrow use case, without most of the flexibility you need to actually customize this for your network and system. Feel free to make use of the reverse engineering docs though - those are accurate to the best of my knowledge.

**Note The Third**: This isn't "complete" in any way shape or form. I built a minimum set of functionality I need to make it work in my application. Not all of it is clean and nice, but it works for what I need it to, and that's what counts :) **If, after all these warnings, you're interested in using this at all, open an issue in this repo and I'll spend some time putting a bow on it.**


After purchasing an EVL-4 "UNO" to get some convential alarm sensors into Home Assistant,
I found the default firmware lacking. The "TPI" interface seems broken and unable to report on zone state,
and trips the alarm on the unit regularly. These are super reliable and well regarded boards when paired with a DSC/Honeywell
alarm panel (which I don't have), so I was a bit disappointed. I'm hoping they fix this in a future firmware update (which by installing this project I've ensured I'll never receive! :) ).

The hardware, however, seems to work great.

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
- [X] Implement a test program to demonstrate the UNO-8 can work independently (in progress!)
- [X] Bring up a basic firmware to test functionality of the EVL-4 (serial, LEDs)
- [X] Add support for the Ethernet PHY/MAC via uIP
- [X] Add support for CoAP to report sensor states and configure the board
- [X] Bring in the UNO-8 library developed as part of the test program to finish things up.


The board-level code is "complete" as far as I need, barring any yet to be found bugs.

I'm working on a better broker than what's in broker/ - I don't like go-coap and how it seems to reply with nonsense sometimes.
