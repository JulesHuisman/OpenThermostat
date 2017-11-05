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
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>

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
    static OpenThermostat Thermostat;

    void connectWIFI();
    void getStartup();
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

    static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

    void checkPayload(uint8_t * payload);

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
