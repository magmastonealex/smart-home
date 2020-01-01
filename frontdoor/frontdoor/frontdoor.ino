#include <PacketSerial.h>
#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

extern "C" {
  #include "gpio.h"
}

extern "C" {
  #include "user_interface.h"
}

WiFiClient net;
MQTTClient client;

#define WLAN_SSID       "IOTAP"
#define WLAN_PASS       ""

#define AIO_SERVER      "10.9.30.1"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "doorsense"
#define AIO_KEY         ""

volatile int motionChanged = 0;
int lastSentState = 0;
uint64_t nextSendTime = 0;
uint64_t nextSleepTime = 0;

void connect() {
  while (WiFi.status() != WL_CONNECTED) { 
    delay(1000);
  }
  
  while (!client.connect("doorsenseClient", AIO_USERNAME, AIO_KEY)) {
    delay(1000);
  }

}

  IPAddress ip(10,9,30,202);   
  IPAddress gateway(10,9,30,1);
  IPAddress subnet(255,255,255,0);   
  
void setup() {
  
  // put your setup code here, to run once:
  delay(10);
  
  pinMode(3, FUNCTION_3);
  pinMode(3, INPUT);

  WiFi.persistent( false );
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  

  WiFi.config(ip, gateway, subnet);

  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    ESP.wdtFeed();
  }
  client.begin(AIO_SERVER, net);

  connect();
  client.publish("sensors/door/frontdoor", "ON");
  for(int i =0; i< 10; i++) {
    client.loop();
    delay(100);
  }
  ESP.deepSleep(0);
}


void loop() {
  
  if (!client.connected()) {
    connect();
  }
  
  client.loop();
  delay(500);  // <- fixes some issues with WiFi stability
  
  int state = digitalRead(3);
  if(state == LOW) {
      client.publish("sensors/door/frontdoor", "OFF");
      delay(100);
      for(int i =0; i< 10; i++) {
        client.loop();
        delay(10);
      }
      
      ESP.deepSleep(0);
  }
  
}
