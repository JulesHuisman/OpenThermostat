#include "OpenThermostatScreen.h";

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

//Switch two values around
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }

OpenThermostatScreen::OpenThermostatScreen()
{

}

//Sets up the SPI connection and sends the initialization bytes to the SSD1306
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

//Clear the whole screen
void OpenThermostatScreen::clearAll()
{
  clear(0,0,128,64);
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
    clear(0,13,128,20);
  } else {
    activeScreen = LOAD_SCREEN;
    loadBarWidth = 0;

    clearAll();

    //Draw the border for the loading bar
    drawLine(14,35,114,35);
    drawLine(114,35,114,57);
    drawLine(14,57,114,57);
    drawLine(14,57,14,35);
  }

  cursorX = (128-(6*len))/2; //Center the text
  cursorY = 13;

  write(text, len, 1);

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

//Draw a screen with a title and a value
void OpenThermostatScreen::valueScreen(char title[], char value[])
{
  activeScreen = VALUE_SCREEN;

  clearAll();
  size_t titleLen = strlen(title);
  size_t valueLen = strlen(value);

  cursorX = (128-(6*titleLen))/2; //Center the text
  cursorY = 8;

  write(title, titleLen, 1);

  uint8_t size;
  if (valueLen > 11) size = 1;
  else size = 2;

  cursorX = (128-((6*size)*valueLen))/2; //Center the text
  cursorY = 30;

  write(value, valueLen, size);
  display();
}

//Draw the home screen
void OpenThermostatScreen::homeScreen(float value)
{
  uint8_t valueLen;

  if (value < 10) valueLen = 3;
  else if (value >= 10 || value < 0) valueLen = 4;

  char valueChar[valueLen];
  dtostrf(value, valueLen, 1, valueChar);

  size_t len = strlen(valueChar);

  //If home screen is already active only redraw the value
  if (activeScreen == HOME_SCREEN) {
    clear(0,18,113,64);
  } else {
    activeScreen = HOME_SCREEN;

    removeSidebarIcon(THERMOMETER_ICON); //Remove the thermometer icon when coming from the menu screen
    clearAll();
    drawSidebar();
  }

  cursorX = 15+(8*(4-valueLen));
  cursorY = 18;

  write(valueChar, len, 4);

  display();
}

//Add a icon at the bottom of the sidebar
void OpenThermostatScreen::addSidebarIcon(uint8_t icon)
{
  for (uint8_t i = 0; i < sizeof(sidebarIcons); i++)
  {
    //If the icon is already drawn, break
    if (sidebarIcons[i] == icon) break;

    //If an empty spot is found, draw the icon
    else if (sidebarIcons[i] == 0)
    {
      sidebarIcons[i] = icon;
      drawSidebar();
      display();
      break;
    }
  }
}

//Remove a icon from the sidebar
void OpenThermostatScreen::removeSidebarIcon(uint8_t icon)
{
  bool removed = false;
  for (uint8_t i = 0; i < 3; i++)
  {
    //If the icon is found, remove the icon
    if (sidebarIcons[i] == icon)
    {
      sidebarIcons[i] = 0;
      clear(113,(i*22),128,((i+1)*22));

      removed = true;
    }
    //Shift all the icons one place up to fill the gap
    if (removed && i < 2)
    {
      sidebarIcons[i] = sidebarIcons[i+1];
    }
  }
  drawSidebar();
  display();
}

void OpenThermostatScreen::drawSidebar()
{
  //Only draw the sidebar if the homescreen is active
  if (activeScreen != HOME_SCREEN) return;

  //Clear the sidebar
  clear(113,0,128,64);

  for (uint8_t i = 0; i < 3; i++)
  {
    if (sidebarIcons[i] > 0)
    {
      drawIcon(113,(i*22),sidebarIcons[i],2);
    }
  }
}

//Draw a menu
void OpenThermostatScreen::menuScreen(uint8_t active)
{
  activeScreen = MENU_SCREEN;

  clearAll();

  //Animation for scrolling below the bottom of the screen
  if (active > menuTop + 2) {
    menuTop = active - 2;
  }
  else if (active < menuTop) {
    menuTop = active;
  }

  for (uint8_t i = menuTop; i < (menuTop+3); i++)
  {
    cursorX = 10;
    cursorY = 4+((i-menuTop)*23);

    if (i == active)
    {
      fillRect(0,cursorY-2,4,16);
      cursorX = 15;
    }

    if(offlineModeOption != true)
    {
      write(menuItems[i],strlen(menuItems[i]),2);
    } else {
      write(menuItems2[i],strlen(menuItems2[i]),2);
    }
  }

  activeMenu = active;

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

void OpenThermostatScreen::drawIcon(int16_t x, int16_t y, uint8_t icon, uint8_t size) {
  for(int8_t i=0; i<7; i++ ) {
      uint8_t line = pgm_read_byte(icons+((icon-1)*7)+i);
      for(int8_t j=0; j<8; j++, line >>= 1) {
        if(line & 0x1) {
          if(size == 1) drawPixel(x+i, y+j, 1);
          else          fillRect(x+(i*size), y+(j*size), size, size);
        }
      }
  }
}

//Updates the display and sends the screen buffer to the SSD1306
void OpenThermostatScreen::display() {
  sendCommand(0x21);
  sendCommand(0x00);
  sendCommand(0x7F);
  sendCommand(0x22);
  sendCommand(0x00);
  sendCommand(0x07);

  #ifdef SSD1306
    digitalWrite(CS_PIN, HIGH);
    digitalWrite(DC_PIN, HIGH);
    digitalWrite(CS_PIN, LOW);

    for (uint16_t i=0; i<1024; i++)
      SPI.transfer(screenBuffer[i]);
  #endif

  #ifdef SH1106
    sendCommand(0x40 | 0x00);

    for (uint8_t page = 0; page < 64/8; page++) {
      for (uint8_t col = 2; col < 130; col++) {
        sendCommand(0xB0 + page);
        sendCommand(col & 0xF);
        sendCommand(0x10 | (col >> 4));
        digitalWrite(CS_PIN, HIGH);
        digitalWrite(DC_PIN, HIGH);
        digitalWrite(CS_PIN, LOW);
        SPI.transfer(screenBuffer[col-2 + page*128]);
      }
    }
  #endif

  digitalWrite(CS_PIN, HIGH);
}

void OpenThermostatScreen::write(char text[], uint8_t length, uint8_t size)
{
  for (uint8_t i = 0; i < length; i++) {
    //Shift the characters if there is a dot or comma (saves space)
    if (text[i] == '.' || text[i] == ',' || text[i-1] == '.' || text[i-1] == ',' )
    {
      cursorX -= size;
    }
    drawChar(cursorX, cursorY, text[i], size);
    cursorX += size * 6;
  }
}

void OpenThermostatScreen::drawChar(int16_t x, int16_t y, unsigned char c, uint8_t size) {

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

// Sends a byte to the screen
void OpenThermostatScreen::sendCommand(uint8_t byte)
{
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(DC_PIN, LOW);
  digitalWrite(CS_PIN, LOW);

  SPI.transfer(byte);

  digitalWrite(CS_PIN, HIGH);
}
