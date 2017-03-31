#include <OpenThermostat.h>
#include <ESP8266WiFi.h>

OpenThermostat Thermostat;

float count = 0;

void setup()
{
  Thermostat.begin();

  //Get the active SSID name and convert it to a char[]
  String ssid = WiFi.SSID();
  char ssidChar[32];
  strcpy(ssidChar, ssid.c_str());

//  //Start with a load screen for connecting to wifi
//  Thermostat.Screen.loadScreen(ssidChar);
//
//  while (WiFi.status() != WL_CONNECTED)
//  {
//    Thermostat.Screen.loadScreenRefresh();
//    delay(100);
//  }
//  Thermostat.Screen.loadScreen("Connected!");
//  delay(1600);

}

void loop()
{
  Thermostat.Screen.homeScreen(count);
  count += 0.1;
  delay(500);
}
