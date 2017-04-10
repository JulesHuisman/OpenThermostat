/*
  Created by Daan van Driel and Jules Huisman, March 25, 2017.
  Released into the public domain.
  V1.0
*/

#ifndef OpenThermostat_h
#define OpenThermostat_h

#include <ESP8266WiFi.h>;
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
    void getWifiStrength();
    void readTemperature();
    OpenThermostatScreen Screen;
    OpenThermostatDht Dht;
    float tempCorrection;
    uint8_t tempMode;
    float temperature;
};

#endif
