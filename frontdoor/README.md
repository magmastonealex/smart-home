Front Door Sensor
===

I already had a 433mhz door entry sensor, but I wasn't happy with the performance and interference that kept happening. While z-wave sensors would likely work, they're quite expensive.

This project is for an esp8266-based door sensor. It's used to trigger a video recording with my [videolooper](../videolooper) program. I was unhappy with the power consumption of my esp8266, and couldn't get it in and out of sleep mode reliably. So, I drastically simplified the circuit.

It's just an ESP8266, with 2 AA batteries, and a magnetic reed switch wired in series with the battery pack. That means that while the door is closed, there is _no_ power consumption! As the door is opened, power is restored to the ESP8266, and it fires a message on MQTT.