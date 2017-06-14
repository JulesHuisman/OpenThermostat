  /*
  Created by Daan van Driel and Jules Huisman, March 25, 2017.
  Released into the public domain.
  V1.0
*/

#ifndef OpenThermostat_h
#define OpenThermostat_h

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include "OpenThermostatScreen.h"
#include "OpenThermostatDht.h"
#include "include/defines.h"
extern "C" {
 #include "gpio.h"
}

class OpenThermostat
{
  public:
    OpenThermostat();
    void begin();
    void run();
    bool offlineMode;
  private:
    void connectWIFI();
    void getStartupSettings();
    void getSSID();
    void setupAP();
    void runAP();
    void submitForm();
    void getWifiStrength();
    void readTemperature();
    void checkTargetTemperature();
    void readButton();
    void readRotary();
    static void PinA();
    static void PinB();
    void EEPROM_readID();
    float getAvgTemperature();
    void addAvgTemperature(float _temperature);
    void checkHeating();
    void heatingOn(bool _heating);
    void postTemperatureAvg();
    void getSchedule();
    void getSettings();
    void postData(uint8_t type);
    void getData(uint8_t type);
    OpenThermostatScreen Screen;
    OpenThermostatDht Dht;
    ESP8266WebServer webServer;
    unsigned long lastWifiStrengthRead,wifiStrengthReadInterval;
    unsigned long lastTemperatureRead,temperatureReadInterval;
    unsigned long lastTargetTemperatureRead,targetTemperatureReadInterval;
    unsigned long lastButtonRead,buttonReadInterval;
    unsigned long lastTemperaturePost,temperaturePostInterval;
    unsigned long lastTemperatureAvg,temperatureAvgInterval;
    unsigned long lastScheduleGet,scheduleGetInterval;
    unsigned long lastSettingsGet,settingsGetInterval;
    unsigned long lastHeating,heatingInterval;
    unsigned long lastHeatingCheck,heatingCheckInterval;
    unsigned long lastMenuRead, menuInterval;
    int previous;
    int time;
    int scheduleLength;
    float schedule[24][3];
    float restingTemp;
    float targetTemperature;
    bool targetTemperatureChanged;
    float tempCorrection;
    bool heating;
    float startTemperature;
    float dipTemperature;
    unsigned long dipTime;
    unsigned long heatingStartTime;
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
    bool buttonClicked;
    const char* host = "dashboard.open-thermostat.com";
    const int httpsPort = 443;
    const char* fingerprint = "9a fb 23 8b b7 d0 6b ab 3d 21 d9 6e 5e 3a a1 55 84 1d d4 82";
};

#endif
