#include "Arduino.h";
#include "OpenThermostat.h";
#include <ESP8266WiFi.h>;

OpenThermostat::OpenThermostat()
{

}

void OpenThermostat::begin()
{
  Serial.println("Begin");

  Screen.begin();

  Screen.startLoadScreen("Updating");

  WiFi.begin("Jules Wireless", "kartodffelsalat");
  while (WiFi.status() != WL_CONNECTED)
  {
    Screen.addLoadScreen(1);
    delay(100);
  }
}
