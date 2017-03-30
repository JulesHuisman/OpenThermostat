#include "Arduino.h"
#include "OpenThermostat.h"
#include "OpenThermostatScreen.h"


OpenThermostat::OpenThermostat()
{

}

void OpenThermostat::begin()
{
  Serial.println("Begin");

  //Call functions in the screen object
  Screen.begin();
}
