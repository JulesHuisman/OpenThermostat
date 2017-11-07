  /*
  Created by Daan van Driel and Jules Huisman, March 25, 2017.
  Released into the public domain.
*/

#ifndef OpenThermostat_h
#define OpenThermostat_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <sha256.h>

#include <include/defines.h>

#include <Screen.h>
#include <Dht.h>


extern "C" {
 #include "gpio.h"
}

class OpenThermostat
{
  public:
    OpenThermostat();
    void begin();
    void run();

  private:
    void connectWIFI();
    void startup();
    void getSSID();

    void setupAP();
    void runAP();
    void submitForm();

    void getWifiStrength();
    void readTemperature();

    void readButton();
    void readRotary();

    static void PinA();
    static void PinB();

    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

    void genKey();

    bool timerReady(unsigned long timer[]);
    void forceTimer(unsigned long timer[]);

    OpenThermostatScreen Screen;
    OpenThermostatDht Dht;
    ESP8266WebServer webServer;
    DNSServer dnsServer;
    WebSocketsClient webSocket;

    //Setup timer array [last read, read interval]
    unsigned long wifiStrengthTimer[2];
    unsigned long temperatureTimer[2];
    unsigned long rotaryTimer[2];
    unsigned long buttonTimer[2];

    float temperature;
    float targetTemperature;
    float tempCorrection;

    bool unit;
    char SSID[32];

    unsigned long sequenceNumber = 0;

    const char *email  = "huisman.jules@gmail.com";
    const char *secret = "8hk7jgivn5gfghrydjkuld7vjhjdurg6";

    static volatile uint8_t aFlag;
    static volatile uint8_t bFlag;
    static volatile uint8_t encoderPos;
    static volatile uint8_t oldEncPos;
    static volatile uint8_t readingA;
    static volatile uint8_t readingB;

    static long rotaryValue;
    long rotaryValueOld;

    bool rotaryTurning;
};

#endif
