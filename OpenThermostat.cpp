#include "OpenThermostat.h";

OpenThermostat::OpenThermostat()
{
  tempCorrection = -3;
  tempMode = CELCIUS;
}

void OpenThermostat::begin()
{
  Screen.begin();
  dht.begin();

  //Get the active SSID name and convert it to a char[]
  char ssidChar[32];
  String ssid = WiFi.SSID();
  strcpy(ssidChar, ssid.c_str());

  //Start with a load screen for connecting to wifi
  Screen.loadScreen(ssidChar);

  while (WiFi.status() != WL_CONNECTED)
  {
    Screen.loadScreenRefresh();
    delay(100);
  }

  //Show a succes message when connected
  Screen.loadScreen("Connected!");
  delay(1600);

  getWifiStrength();
  readTemperature();
}

//The loop function of the library
void OpenThermostat::run()
{
  getWifiStrength();
  readTemperature();
  delay(5000);
}

//Get the wifi strength and draw it as an icon when on the home screen
void OpenThermostat::getWifiStrength()
{
  uint8_t strength = map(WiFi.RSSI(),-80,-67,1,3);
  strength = constrain(strength,1,3);

  Screen.sidebarIcons[0] = strength;
  Screen.drawSidebar();
}

//Reads the current temperature and prints it to the home screen
void OpenThermostat::readTemperature()
{
  float t = dht.readTemperature(tempMode);
  t += tempCorrection;

  Screen.homeScreen(t);
}
