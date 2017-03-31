#include "Arduino.h";
#include "OpenThermostat.h";
#include <ESP8266WiFi.h>;

OpenThermostat::OpenThermostat()
{

}

void OpenThermostat::begin()
{
  Screen.begin();
}
