//Define the pins the screen is connected to, the other two screen pins are SPI hardware pins
#define CS_PIN      15
#define DC_PIN      3
#define RST_PIN     5
#define HEATING_PIN 12

//DHT pin
#define DHT_PIN 2

//Rotary encoder pins
#define ROTA_PIN 0
#define ROTB_PIN 4
#define BUT_PIN  A0

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
#define NO_INTERNET_ICON 8

//Define the different temperature modes
#define CELCIUS    0
#define FAHRENHEIT 1

//Define all the main menu items
#define MAIN_MENU_RETURN  0
#define MAIN_MENU_UPDATE  1

//Define menu lengths
#define MAIN_MENU_LENGTH 4

//Define timer array indices
#define LAST_READ     0
#define READ_INTERVAL 1
