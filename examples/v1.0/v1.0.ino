#include <OpenThermostat.h>

OpenThermostat Thermostat;

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Thermostat.begin();
}

void loop()
{
  Thermostat.run();
}
