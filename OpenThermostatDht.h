/* OpenThermostatDht library

  MIT license
  written by Adafruit Industries

  Modified to suit the OpenThermosat Library
*/

#ifndef OpenThermostatDht_H
#define OpenThermostatDht_H

#include "Arduino.h"
#include "include/defines.h"

// Define types of sensors.
#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21


class OpenThermostatDht {
  public:
   OpenThermostatDht();
   void begin(void);
   float readTemperature(bool S=false, bool force=false);
   float convertCtoF(float);
   float convertFtoC(float);
   float readHumidity(bool force=false);
   boolean read(bool force=false);

 private:
  uint8_t data[5];
  uint8_t _type;
  uint32_t _lastreadtime, _maxcycles;
  bool _lastresult;

  uint32_t expectPulse(bool level);

};

class InterruptLock {
  public:
   InterruptLock() {
    noInterrupts();
   }
   ~InterruptLock() {
    interrupts();
   }

};

#endif
