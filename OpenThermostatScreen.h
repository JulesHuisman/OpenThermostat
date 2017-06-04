/*
  Created by Daan van Driel and Jules Huisman, March 25, 2017.
  Released into the public domain.
  V1.0

  Based on the Adafruit library for the SSD1306
  https://github.com/adafruit/Adafruit_SSD1306

  And SH1106 driver library
  https://github.com/rene-mt/esp8266-oled-sh1106
*/

#ifndef Screen_h
#define Screen_h

#include <SPI.h>;
#include "Arduino.h";
#include "include/defines.h"
#include "include/graphics.h"
extern "C" {
  #include<user_interface.h>
}

//#define SSD1306
#define SH1106

class OpenThermostatScreen
{
  public:
    OpenThermostatScreen();
    void begin();
    void display();
    void loadScreen(char text[]);
    void loadScreenRefresh();
    void valueScreen(char title[], char value[]);
    void homeScreen(float value);
    void addSidebarIcon(uint8_t icon);
    void removeSidebarIcon(uint8_t icon);
    void drawSidebar();
    void menuScreen(uint8_t active);
    void clearAll();
    void clear(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    uint8_t activeScreen;
    uint8_t activeMenu;
    uint8_t sidebarIcons[3];
    uint8_t menuLength;
    char *menuItems[6] = {"Return", "Update","ID Code","Unit","Version"};
    char *menuItems2[3] = {"Offline","AP",""};
    bool offlineModeOption;
  private:
    void write(char text[], uint8_t length, uint8_t size);
    void drawPixel(int16_t x, int16_t y, uint8_t color);
    void drawIcon(int16_t x, int16_t y, uint8_t icon, uint8_t size);
    void drawChar(int16_t x, int16_t y, unsigned char c, uint8_t size);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void sendCommand(uint8_t byte);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h);
    uint8_t screenBuffer[1024];
    uint8_t cursorX, cursorY;
    uint8_t loadBarWidth;
    uint8_t menuTop;
};

#endif
