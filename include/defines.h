
//Define the pins the screen is connected to, the other two screen pins are SPI hardware pins
#define CS_PIN      15
#define DC_PIN      5
#define RST_PIN     0
#define HEATING_PIN 2

//Define the different screens that exist
#define LOAD_SCREEN  1
#define HOME_SCREEN  2
#define MENU_SCREEN  3
#define VALUE_SCREEN 4

//Define the different custom icons
#define WIFI_ICON_3      1
#define WIFI_ICON_2      2
#define WIFI_ICON_1      3
#define UPDATE_ICON      4
#define DEGREE_ICON      5
#define HEATING_ICON     6
#define THERMOMETER_ICON 7

//Define the different temperature modes
#define CELCIUS    0
#define FAHRENHEIT 1

//DHT pin
#define DHT_PIN 3

//Rotary encoder pins
#define ROTA_PIN 4
#define ROTB_PIN 12
#define BUT_PIN  A0

//Define all the main menu items
#define MAIN_MENU_RETURN  0
#define MAIN_MENU_UPDATES 1
#define MAIN_MENU_ID      2
#define MAIN_MENU_UNIT    3
#define MAIN_MENU_VERSION 4
//Define all the offlineMode menu items
#define OFFLINE_MODE  0
#define ACCES_POINT   1

#define MAIN_MENU_LENGTH   5
#define OFFLINE_MODE_MENU_LENGTH   2

//Define the different type of HTTP posts and gets
#define TEMPERATURE_POST     0
#define HEATING_POST         1
#define GET_SCHEDULE         2
#define GET_SETTINGS         3
