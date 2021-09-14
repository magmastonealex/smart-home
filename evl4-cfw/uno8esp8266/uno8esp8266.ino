// Reports state of UNO-8 inputs over MQTT.
// For me, a stopgap before I get the main EVL-4 board brought up.
// This is kinda crappy code but it does the job for now.
#include <Wire.h>
#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Defines WLAN_SSID and WLAN_PASS, plus MQTT_USER, MQTT_KEY
#include "secrets.h"

WiFiClient net;
MQTTClient client;

uint8_t values[8];
// What's the "normal" value for this channel? Anything +- this value is acceptable and will leave the channel in "off" state. Deviations will be "ON".
// It would be nice to do this config dynamically via a web interface.
uint8_t expected_values[8] = {0x6E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// will the channel trigger high or low? If it goes the opposite direction, it will be reported as a tamper as well as triggered.
uint8_t trigger_directions[8] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void connect() {
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    ESP.wdtFeed();
  }
  client.setWill("alarm/channels/available", "offline", true, 0);
  while (!client.connect("alarmSenseClient", MQTT_USER, MQTT_KEY)) {
    delay(100);
    ESP.wdtFeed();
  }
  client.publish("alarm/channels/available", "online", true, 0);
}



void setup() {
  // put your setup code here, to run once:
  //Wire.setSpeed(100000);
  Serial.begin(115200);
  client.begin("10.102.40.20", net);
  delay(100);
  connect();

  Serial.println("Alive and connected!");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Wire.begin();
}


uint8_t iters = 0;

unsigned long lastPoll = 0;
unsigned long lastReadSuccess = 0;
unsigned long lastMqttBlast = 0;

uint8_t currentStates[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t currentTampers[8] = {0, 0, 0, 0, 0, 0, 0, 0};

char topicName[255];
char topicValue[10];

void mqttSendAll() {

  for (int i = 0; i < 8; i++) {
    snprintf(topicName, 255, "alarm/channels/%d/status", i+1);
    client.publish(topicName, currentStates[i] ? "ON" : "OFF", true, 0);
  }

  for (int i = 0; i < 8; i++) {
    snprintf(topicName, 255, "alarm/channels/%d/tamper", i+1);
    client.publish(topicName, currentTampers[i] ? "ON" : "OFF", true, 0);
  }
}

void recordChanges() {
  for (int i = 0; i<8; i++) {
      uint8_t val = values[i];
      uint8_t expected = expected_values[i];
      uint8_t trigger_direction = trigger_directions[i];
      uint8_t changed = 0;
      snprintf(topicName, 255, "Examining %d, current %x, expected %x, triggerdir %x", i, val, expected, trigger_direction);
      Serial.println(topicName);
      
      if (expected > 250) {
        expected = 250;
      }
      if (expected < 5) {
        expected = 5;
      }
      if ((val > (expected+5)) || (val < (expected-5))) {
        snprintf(topicName, 255, "triggering %d", i);
        Serial.println(topicName);
        if(currentStates[i] != 1) {
          currentStates[i] = 1;
          changed = 1;
        }
        uint8_t tamper = 0;
        // Tamper direction?
        if (val > (expected+5)) {
             if (trigger_direction == 0x00) {
              tamper = 1;
             } else {
              tamper = 0;
             }
        } else {
          if (trigger_direction == 0xFF) {
              tamper = 1;
             } else {
              tamper = 0;
             }
        }
        if (tamper == 1) {
          if(currentTampers[i] != 1) {
            currentTampers[i] = 1;
            changed = 1;
          }
        } else {
          if(currentTampers[i] != 0) {
            currentTampers[i] = 0;
            changed = 1;
          }
        }
      }
      else {
        snprintf(topicName, 255, "restoring %d", i);
        Serial.println(topicName);
        if(currentStates[i] != 0) {
          currentStates[i] = 0;
          changed = 1;
        }
        if(currentTampers[i] != 0) {
          currentTampers[i] = 0;
          changed = 1;
        }
      }

      if (changed) {
          snprintf(topicName, 255, "%d had changes", i);
        Serial.println(topicName);
          snprintf(topicName, 255, "alarm/channels/%d/tamper", i+1);
          client.publish(topicName, currentTampers[i] ? "ON" : "OFF");
          snprintf(topicName, 255, "alarm/channels/%d/status", i+1);
          client.publish(topicName, currentStates[i] ? "ON" : "OFF");
          snprintf(topicName, 255, "alarm/channels/%d/value", i+1);
          snprintf(topicValue, 10, "%x", values[i]);
          client.publish(topicName, topicValue);
      }
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  if (WiFi.status() != WL_CONNECTED || !client.connected()) {
    connect();
  }
  client.loop();
  delay(5);
  ArduinoOTA.handle();

  unsigned long elapsed = millis() - lastMqttBlast;
  if (elapsed > 30000) {
      mqttSendAll();
      lastMqttBlast = millis();
  }

  elapsed = millis() - lastPoll;
  if (lastPoll == 0 || (elapsed > 50)) {
    // Give the uno8 time to settle after startup.
    if (iters < 20) {
      iters++;
      return;
    }
    Wire.beginTransmission(0x20);
    Wire.write((byte)0x10);
    Wire.endTransmission();

    Wire.requestFrom(0x20, 8);

    if (Wire.available() < 8) {
      Serial.println("uno8 sent too few bytes");
      while (Wire.available()) {
        Wire.read();
      }
      delay(100);
      return;
    }
    
    uint8_t changes = 0;
    for (int i = 0; i < 8; i++) {
      uint8_t value = Wire.read();
      if (values[i] != value) {
        changes = 1;
      }
      values[i] = value;
    }



    if (changes) {
      Serial.print("new vals");
      for (int i =0; i < 8; i++) {
        Serial.print(values[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      recordChanges();
    }
    lastPoll = millis();
  }

}
