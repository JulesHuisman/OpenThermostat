#include <OpenThermostat.h>

OpenThermostat Thermostat;

void setup()
{
  Serial.begin(115200);
  Thermostat.begin();
}

void loop()
{
  Thermostat.run();
}
