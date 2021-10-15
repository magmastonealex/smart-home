#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <SPI.h>
#include "epd7in5_V2.h"
#include "imagedata.h"

// Defines WLAN_SSID and WLAN_PASS.
#include "secret.h"

uint8_t screenBuf[800] = { 0 };

void connect() {
  Serial.println("connect");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    ESP.wdtFeed();
  }
Serial.println("connected");
}

HTTPClient http;
WiFiClient wifiClient;

void redraw() {
    Epd epd;
    Serial.print("e-Paper init \r\n ");
    if (epd.Init() != 0) {
        Serial.print("e-Paper init failed\r\n ");
        return;
    }

    Serial.println("Fetching img...");
    http.begin(wifiClient, "http://10.9.30.1:8991/getimg");

    Serial.println("GET");
    int httpCode = http.GET();
    Serial.print("[HTTP] GET... code: ");
    Serial.println(httpCode);
    if (httpCode != 200) {
      Serial.println("Not 200 OK... Bailing...");
      return;
    }
    int len = http.getSize();
    if (len != 48000) {
      Serial.println("Bad size... Bailing...");
      return;
    }

    Serial.println("Reading and blitting...");
    WiFiClient * stream = http.getStreamPtr();

    // read all data from server
    epd.SendCommand(0x13);
    while (http.connected() && (len > 0 || len == -1)) {
      // read up to 800 bytes
      int c = stream->readBytes(screenBuf, std::min((size_t)len, sizeof(screenBuf)));
      ESP.wdtFeed();
      if (!c) {
        Serial.println("read timeout");
        continue;
      }
      //Serial.printf("Read %d \r\n", c);

      // write it to Serial
      for(int i = 0; i < c; i++) {
        epd.SendData(~screenBuf[i]);
      }

      //Serial.printf("Sent %d, %d remains \r\n", c, len - c);

      if (len > 0) {
        len -= c;
      }
    }
    epd.SendCommand(0x12);
    Serial.println("Done!");
    http.end();
    Serial.println("ended");    

    delay(100);
    epd.WaitUntilIdle();
    Serial.println("idle done");
    
    ESP.wdtFeed();
    Serial.print("e-Paper Clear\r\n ");
    //epd.Clear();

    epd.Sleep();
    Serial.println("sleep return;");
}

unsigned long lastRedraw = 0;

void setup() {
    ESP.wdtEnable(3000);
  // put your setup code here, to run once:
    Serial.begin(9600);
    connect();  

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
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long elapsed = millis() - lastRedraw;
  if (lastRedraw == 0 || (elapsed > 1200000)) {
    redraw();
    lastRedraw = millis();
  }
  ArduinoOTA.handle();
  delay(100);
}
