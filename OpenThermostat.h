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
#include <WiFiClientSecure.h>;
#include <EEPROM.h>;
#include <ArduinoJson.h>;
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
    void getStartupSettings();
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
    float getAvgTemperature();
    void addAvgTemperature(float _temperature);
    void postTemperatureAvg();
    void postTemperature();
    void getSettings();
    void postData(uint8_t type);
    void getData(uint8_t type);
    OpenThermostatScreen Screen;
    OpenThermostatDht Dht;
    ESP8266WebServer webServer;
    StaticJsonBuffer<400> jsonBuffer;
    unsigned long lastWifiStrengthRead,wifiStrengthReadInterval;
    unsigned long lastTemperatureRead,temperatureReadInterval;
    unsigned long lasttargetTemperatureRead,targetTemperatureInterval;
    unsigned long lastButtonRead,buttonReadInterval;
    unsigned long lastTemperaturePost,temperaturePostInterval;
    unsigned long lastTemperatureAvg,temperatureAvgInterval;
    unsigned long lastTemperatureGet,temperatureGetInterval;
    unsigned long lastSettingsGet,settingsGetInterval;
    int previous;
    float outdoorTemp;
    float targetTemp;
    float targetTempWeb;
    float targetTempWebOld;
    float tempCorrection;
    int minTemp;
    int maxTemp;
    uint8_t tempMode;
    float temperature;
    float temperatureArray[15];
    char SSID[32];
    char *idCode = "0000000000000000";
    char *version = "0.0.0";
    uint8_t updateAvailable;
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
