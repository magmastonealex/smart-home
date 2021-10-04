/**
 *  @filename   :   epd7in5-demo.ino
 *  @brief      :   7.5inch e-paper display demo
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     July 10 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    ESP.wdtFeed();
  }
}

void redraw() {
    Epd epd;
    Serial.print("e-Paper init \r\n ");
    if (epd.Init() != 0) {
        Serial.print("e-Paper init failed\r\n ");
        return;
    }

    Serial.println("Fetching img...");
    HTTPClient http;
    http.begin("http://10.102.40.67:8080/getimg");

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
    

    delay(100);
    epd.WaitUntilIdle();

    
    ESP.wdtFeed();
    Serial.print("e-Paper Clear\r\n ");
    //epd.Clear();

    epd.Sleep();
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

uint8_t CARD_DATA[12] = {0};
uint8_t CARD_CHARS = 0;
uint8_t CARD_STATE = 0;

#define CARD_STATE_IDLE 0
#define CARD_STATE_READING 1

String serverName = "http://10.102.40.67:8080/carddata?cardId=";

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long elapsed = millis() - lastRedraw;
  if (lastRedraw == 0 || (elapsed > 600000)) {
    redraw();
    lastRedraw = millis();
  }
  ArduinoOTA.handle();
  server.handleClient();
  delay(10);
}
