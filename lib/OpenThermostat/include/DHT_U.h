/*
  DHT Temperature & Humidity Unified Sensor Library
  Copyright (c) 2014 Adafruit Industries
  Author: Tony DiCola

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  Modified to suit the OpenThermosat Library
*/

#ifndef DHT_U_H
#define DHT_U_H

#include <../OpenThermostatDht.h>

#include "Arduino.h"
#include "Print.h"

typedef struct
{
    char     name[12];                        /**< sensor name */
    int32_t  version;                         /**< version of the hardware + driver */
    int32_t  sensor_id;                       /**< unique sensor identifier */
    int32_t  type;                            /**< this sensor's type (ex. SENSOR_TYPE_LIGHT) */
    float    max_value;                       /**< maximum value of this sensor's value in SI units */
    float    min_value;                       /**< minimum value of this sensor's value in SI units */
    float    resolution;                      /**< smallest difference between two values reported by this sensor */
    int32_t  min_delay;                       /**< min delay in microseconds between events. zero = not a constant rate */
} sensor_t;

class Adafruit_Sensor {
 public:
  // Constructor(s)
  Adafruit_Sensor() {}
  virtual ~Adafruit_Sensor() {}

  // These must be defined by the subclass
  virtual void enableAutoRange(bool enabled) {};
  virtual bool getEvent(sensors_event_t*) = 0;
  virtual void getSensor(sensor_t*) = 0;

 private:
  bool _autoRange;
};

#define DHT_SENSOR_VERSION 1

class DHT_Unified {
public:
  DHT_Unified(uint8_t pin, uint8_t type, uint8_t count=6, int32_t tempSensorId=-1, int32_t humiditySensorId=-1);
  void begin();

  class Temperature : public Adafruit_Sensor {
  public:
    Temperature(DHT_Unified* parent, int32_t id);
    bool getEvent(sensors_event_t* event);
    void getSensor(sensor_t* sensor);

  private:
    DHT_Unified* _parent;
    int32_t _id;

  };

  class Humidity : public Adafruit_Sensor {
  public:
    Humidity(DHT_Unified* parent, int32_t id);
    bool getEvent(sensors_event_t* event);
    void getSensor(sensor_t* sensor);

  private:
    DHT_Unified* _parent;
    int32_t _id;

  };

  Temperature temperature() {
    return _temp;
  }

  Humidity humidity() {
    return _humidity;
  }

private:
  OpenThermostatDht _dht;
  uint8_t _type;
  Temperature _temp;
  Humidity _humidity;

  void setName(sensor_t* sensor);
  void setMinDelay(sensor_t* sensor);

};

#endif
