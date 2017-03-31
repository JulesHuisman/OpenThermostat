#include "Arduino.h";
#include "OpenThermostatScreen.h";
#include <SPI.h>;

//Include font and graphics
#include "include/graphics.h";

//Define the pins the screen is connected to, the other two pins are SPI hardware pins
#define CS_PIN  15
#define DC_PIN  5
#define RST_PIN 0

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

//Switch two values around
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }

OpenThermostatScreen::OpenThermostatScreen()
{

}

//Sets up the SPI connecting and sends the initialization bytes to the SSD1306
void OpenThermostatScreen::begin()
{
  //The commands to send to the SSD1306 on startup
  static const uint8_t initCommandArray[26] = {0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40|0x00,0x8D,0x14,0x20,0x00,0xA0|0x1,0xC8,0xDA,0x12,0x81,0xCF,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0x2E,0xAF};

  pinMode(DC_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);

  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  digitalWrite(RST_PIN, HIGH);
  digitalWrite(RST_PIN, LOW);
  digitalWrite(RST_PIN, HIGH);

  //Send all the initialization bytes to the SSD1306
  for (uint8_t i = 0; i < sizeof(initCommandArray); i++)
    sendCommand(initCommandArray[i]);
}

void OpenThermostatScreen::clearAll()
{
  clear(0,0,128,64);
  DEBUG_PRINTLN(F("Clear whole screen"));
}

void OpenThermostatScreen::clear(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  for(uint8_t c = x0; c < x1; c++) {
    for(uint8_t r = y0; r < y1; r++) {
      drawPixel(c,r,0);
    }
  }
}

//Create a screen with a textfield and a loading bar
void OpenThermostatScreen::loadScreen(char text[])
{
  size_t len = strlen(text);

  //If load screen is already active only redraw the text
  if (activeScreen == LOAD_SCREEN) {
    clear(0,13,127,20);
  } else {
    clearAll();

    loadBarWidth = 0;
    activeScreen = LOAD_SCREEN;

    //Draw the border for the loading bar
    drawLine(14,35,114,35);
    drawLine(114,35,114,57);
    drawLine(14,57,114,57);
    drawLine(14,57,14,35);
  }

  cursorX = (128-(6*len))/2; //Center the text
  cursorY = 13;

  write(text, 1);

  display();
}

//Add 1 width to the right of the loading bar
void OpenThermostatScreen::loadScreenRefresh()
{
  loadBarWidth++;

  //Reset the loading bar if it reaches the end
  if (loadBarWidth > 95)
  {
    loadBarWidth = 0;
    clear(17,38,113,55);
  }

  fillRect(17,38,loadBarWidth,17);
  display();
}

//Draw the home screen
void OpenThermostatScreen::homeScreen(float value)
{
  char valueChar[4];
  dtostrf(value, 3, 2, valueChar);

  //If home screen is already active only redraw the value
  if (activeScreen == HOME_SCREEN) {
    clear(0,7,127,64);
  } else {
    clearAll();

    activeScreen = HOME_SCREEN;

    //drawHeader();
  }

  cursorX = (128-(23*4))/2; //Center the text
  cursorY = 24;

  write(valueChar, 4);

// ============================================== TEMP ==============================================

drawChar(20,4,(char)223,1);

    cursorX = 0;
    cursorY = 2;

    char temp[] = "20.4";
    write(temp, 1);

// ============================================== TEMP ==============================================

  display();
}

//Draw a pixel at a x,y position
//Color 0 = off
//Color 1 = on
void OpenThermostatScreen::drawPixel(int16_t x, int16_t y, uint8_t color) {
  switch (color)
  {
    case 1: screenBuffer[x+ (y/8)*128] |=  (1 << (y&7)); break;
    case 0: screenBuffer[x+ (y/8)*128] &= ~(1 << (y&7)); break;
  }
}

//Updates the display and sends the screen buffer to the SSD1306
void OpenThermostatScreen::display() {
  sendCommand(0x21);
  sendCommand(0);
  sendCommand(127);
  sendCommand(0x22);
  sendCommand(0);
  sendCommand(7);

  digitalWrite(CS_PIN, HIGH);
  digitalWrite(DC_PIN, HIGH);
  digitalWrite(CS_PIN, LOW);

  for (uint16_t i=0; i<1024; i++)
    SPI.transfer(screenBuffer[i]);

  digitalWrite(CS_PIN, HIGH);
}

void OpenThermostatScreen::write(char text[], uint8_t size)
{
  for (uint8_t i = 0; i < sizeof(text); i++) {
    drawChar(cursorX, cursorY, text[i], size);
    cursorX += size * 6;
  }
}

void OpenThermostatScreen::drawChar(int16_t x, int16_t y, unsigned char c, uint8_t size) {

  if(c >= 176) c++;

  for(int8_t i=0; i<6; i++ ) {
      uint8_t line;
      if(i < 5) line = pgm_read_byte(font+(c*5)+i);
      else      line = 0x0;
      for(int8_t j=0; j<8; j++, line >>= 1) {
        if(line & 0x1) {
          if(size == 1) drawPixel(x+i, y+j,1);
          else          fillRect(x+(i*size), y+(j*size), size, size);
        }
      }
  }
}

//Use the drawLine function to fill a rectangle with lines
void OpenThermostatScreen::fillRect(int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t i=x; i<x+w; i++) {
        drawLine(i, y, i, y+h-1);
    }
}

//Draw a line from x0,y0 to x1,y1
void OpenThermostatScreen::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            drawPixel(y0, x0,1);
        } else {
            drawPixel(x0, y0,1);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

// Sends a byte to the SSD1306
void OpenThermostatScreen::sendCommand(uint8_t byte)
 {
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(DC_PIN, LOW);
  digitalWrite(CS_PIN, LOW);

  SPI.transfer(byte);

  digitalWrite(CS_PIN, HIGH);
}
