/*
  Created by Daan van Driel and Jules Huisman, March 25, 2017.
  Released into the public domain.
*/

#ifndef OpenThermostat_h
#define OpenThermostat_h

#include "Arduino.h"

//Comment to remove debugging serial prints
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(...) { Serial.print(__VA_ARGS__); }
  #define DEBUG_PRINTLN(...) { Serial.println(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...) {}
  #define DEBUG_PRINTLN(...) {}
#endif

class OpenThermostat
{
  public:
    OpenThermostat();
  private:
    //
};

#endif
