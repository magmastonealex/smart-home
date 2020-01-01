mqtt_cacher
===

This program subscribes to MQTT topics being published by IoT devices. It can help track "liveness", especially on devices that regularly report some form of heartbeat.

It's configured in config.json, and exposes an HTTP server that dumps out all "last heartbeat" timestamps for known devices at the "/getInfo" endpoint. 
