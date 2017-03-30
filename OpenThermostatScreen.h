/*
  Created by Daan van Driel and Jules Huisman, March 25, 2017.
  Released into the public domain.
  V1.0

  Based on the Adafruit library for the SSD1306
  https://github.com/adafruit/Adafruit_SSD1306
*/

#ifndef OpenThermostatScreen_h
#define OpenThermostatScreen_h

#include "Arduino.h";

//Comment to remove debugging serial prints
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(...) { Serial.print(__VA_ARGS__); }
  #define DEBUG_PRINTLN(...) { Serial.println(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...) {}
  #define DEBUG_PRINTLN(...) {}
#endif

class OpenThermostatScreen
{
  public:
    OpenThermostatScreen();
    void begin();
    void display();
    void startLoadScreen(char text[]);
    void addLoadScreen(uint8_t width);
    void clearAll();
    void clear(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
  private:
    void write(uint8_t c, uint8_t size);
    void drawPixel(int16_t x, int16_t y, uint8_t color);
    void drawChar(int16_t x, int16_t y, unsigned char c, uint8_t size);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void sendCommand(uint8_t byte);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h);
    uint8_t screenBuffer[1024];
    uint8_t cursorX;
    uint8_t cursorY;
    uint8_t loadingTime;
    uint8_t loadingBarWidth;
};

#endif
