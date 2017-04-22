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
#include <WiFiClientSecure.h>
#include <EEPROM.h>;
#include "Arduino.h";
#include "OpenThermostatScreen.h";
#include "OpenThermostatDht.h";
#include "include/defines.h";
extern "C" {
#include "gpio.h"
}

#define DEBUG_ESP_SSL
#define DEBUG_SSL

class OpenThermostat
{
  public:
    OpenThermostat();
    void begin();
    void run();
    void setID(char ID[]);
  private:
    void connectWIFI();
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
    void EEPROM_writeID(int adress, char Str[]);
    void EEPROM_readID(int adress);
    void postTemperature();
    void postData(uint8_t type);
    OpenThermostatScreen Screen;
    OpenThermostatDht Dht;
    ESP8266WebServer webServer;
    unsigned long lastWifiStrengthRead;
    unsigned long wifiStrengthReadInterval;
    unsigned long lastTemperatureRead;
    unsigned long temperatureReadInterval;
    unsigned long lastSetTemperatureRead;
    unsigned long setTemperatureInterval;
    unsigned long lastButtonRead;
    unsigned long buttonReadInterval;
    unsigned long lastTemperaturePost;
    unsigned long temperaturePostInterval;
    int previous;
    float setTemp;
    float tempCorrection;
    int minTemp;
    int maxTemp;
    uint8_t tempMode;
    float temperature;
    char SSID[32];
    char *idCode = "000000000";
    int8_t activeMenu;
    static volatile uint8_t aFlag;
    static volatile uint8_t bFlag;
    static volatile uint8_t encoderPos;
    static volatile uint8_t oldEncPos;
    static volatile uint8_t readingA;
    static volatile uint8_t readingB;
    static long rotaryValue;
    long rotaryValueOld;
    bool accesPointActive;
    const char* host = "dashboard.open-thermostat.com";
    const int httpsPort = 443;
    const char* fingerprint = "9a fb 23 8b b7 d0 6b ab 3d 21 d9 6e 5e 3a a1 55 84 1d d4 82";
};

#endif
