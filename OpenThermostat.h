/*
  Created by Daan van Driel and Jules Huisman, March 25, 2017.
  Released into the public domain.
  V1.0
*/

#ifndef OpenThermostat_h
#define OpenThermostat_h

#include <ESP8266WiFi.h>;
#include <DNSServer.h>;
#include <ESP8266WebServer.h>;
#include "Arduino.h";
#include "OpenThermostatScreen.h";
#include "OpenThermostatDht.h";
#include "include/defines.h";

class OpenThermostat
{
  public:
    OpenThermostat();
    void begin();
    void run();
  private:
    void connectWIFI();
    void getSSID();
    void setupAP();
    void runAP();
    void submitForm();
    void getWifiStrength();
    void readTemperature();
    static void PinA();
    static void PinB();
    void readButton();
    OpenThermostatScreen Screen;
    OpenThermostatDht Dht;
    ESP8266WebServer webServer;
    static float targetTemp;
    float tempCorrection;
    uint8_t tempMode;
    float temperature;
    char SSID[32];
    bool accesPointActive;
    static volatile uint8_t aFlag;
    static volatile uint8_t bFlag;
    static volatile uint8_t encoderPos;
    static volatile uint8_t oldEncPos;
    static volatile uint8_t readingA;
    static volatile uint8_t readingB;
    bool buttonPressed;
};

#endif
