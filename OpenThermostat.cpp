#include "OpenThermostat.h";

long OpenThermostat::rotaryValue = 0;
volatile uint8_t OpenThermostat::aFlag = 0;
volatile uint8_t OpenThermostat::bFlag = 0;
volatile uint8_t OpenThermostat::encoderPos = 0;
volatile uint8_t OpenThermostat::oldEncPos = 0;
volatile uint8_t OpenThermostat::readingA;
volatile uint8_t OpenThermostat::readingB;

OpenThermostat::OpenThermostat()
{
  tempCorrection = 0;
  tempMode = CELCIUS;

  minTemp = 0;
  maxTemp = 25;

  wifiStrengthReadInterval = 60000; //How often to read wifi strength (60 s)
  temperatureReadInterval = 4000; //How often to get the indoor temperature (10 s)
  targetTemperatureInterval = 2500; //How long to display the set temperature (2.5 s)
  temperaturePostInterval = 900000; //The time between temperature posts (15 min)
  temperatureAvgInterval = 60000; //The time between adding to average temperature (1 min)
  temperatureGetInterval = 60000; //How often to post and get the temperature and target temperature (1 min)
  settingsGetInterval = 300000; //How often toget the current settings and updates (5 min)
  buttonReadInterval = 150; //Debounce delay for readButton() (150 ms)
  previous = 0;

  lastWifiStrengthRead = -wifiStrengthReadInterval; //Forces get wifi strength on startup
  lastTemperatureRead  = -temperatureReadInterval; //Forces get temperature on startup
  lastTemperaturePost  = -temperaturePostInterval; //Forces posting the temperature on startup
  lastTemperatureAvg   = -temperatureAvgInterval; //Forces adding to average temperature on startup
  lastSettingsGet      = -settingsGetInterval; //Forces getting the settings on startup
}

void OpenThermostat::begin()
{
  Screen.begin();
  Dht.begin();
  EEPROM.begin(9);

  //Get the ID code for this thermostat and save it in a variable
  EEPROM_readID(0);

  //PinModes and intterupts for the rotary encoder
  pinMode(ROTA_PIN, INPUT_PULLUP);
  pinMode(ROTB_PIN, INPUT_PULLUP);
  attachInterrupt(ROTA_PIN, PinA, RISING);
  attachInterrupt(ROTB_PIN, PinB, RISING);


  connectWIFI();

  Screen.homeScreen(0);

}

//The loop function of the library
void OpenThermostat::run()
{
   getWifiStrength();
   readTemperature();
   postTemperatureAvg();
   postTemperature();
   getSettings();
   readRotary();
   readButton();
}

//Writes an ID code in the EEPROM
void OpenThermostat::setID(char ID[])
{
  EEPROM.begin(9);
  EEPROM_writeID(0,ID);
  EEPROM_readID(0);
}

void OpenThermostat::connectWIFI()
{
  //Start in station mode
  WiFi.mode(WIFI_STA);

  //Save the wifi credentials after restart of the esp8266
  WiFi.persistent(true);

  //Show a load screen with the active SSID
  getSSID();
  Screen.loadScreen(SSID);

  //Loop until connected
  for (uint8_t i = 0; i < 96; i++)
  {
    Screen.loadScreenRefresh();
    delay(100);

    if (WiFi.status() == WL_CONNECTED) break;
  }

  switch (WiFi.status()) {
    //Show a succes message when connected
    case WL_CONNECTED:
      getStartupSettings();
      break;

    //If unable to connect start an access point
    case WL_NO_SSID_AVAIL:
      setupAP();

      //Loop the access point to poll for user input
      while(accesPointActive) runAP();
      break;
  }
}

void OpenThermostat::getStartupSettings()
{
  Screen.loadScreen("Fetching settings");
  getData(GET_STARTUP_SETTINGS);
}

//Get the active SSID name and convert it to a character array
void OpenThermostat::getSSID()
{
  String ssid = WiFi.SSID();
  strcpy(SSID, ssid.c_str());
}

//Setup the esp8266 as an access point
void OpenThermostat::setupAP()
{
  IPAddress AccessIP = IPAddress(10, 10, 10, 1);
  IPAddress AccessDG = IPAddress(10, 10, 10, 1);
  IPAddress AccessSN = IPAddress(255, 255, 255, 0);

  String HTML =
  F("<!DOCTYPE html>"
  "<html>"
    "<head>"
      "<title>OpenThermostat Setup</title>"
      "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'/>"
      "<meta name='theme-color' content='#227bca'>"
      "<style>"
        "body{background-color:#1e272f;font-family:arial;color:#ccdfef;text-align: center;}"
        "td{padding: 0, 10px;}"
        "table{margin: 0 auto;width:60%}"
        "::-webkit-input-placeholder {color:#525e68;}"
        "input{border:none;}"
        "input{background-color:transparent;border-bottom:2px solid #227bca;color:white;width:100%;font-size:18px;padding:5px 0px;margin-bottom:15px;transition: all .2s}"
        "input:focus{outline:none;border-color:#59a9ef;color:#fff}"
        "input[type=submit]{background-color:#227bca;width:auto;padding:10px 40px 8px 40px;margin:50px auto 0 auto;border-radius: 20px; font-size:14px;box-shadow: 0px 0px 5px #161b1f;}"
        "h1{text-align: center;margin:30px 0 50px 0;}"
        "p{font-size:12px;margin-top: 20px;}"
      "</style>"
    "</head>"
    "<body>"
      "<p>OpenThermostat</p>"
      "<h1>Network setup</h1>"
      "<form action='/submit' method='POST'>"
        "<table>"
          "<tr><td><input type='text' name='ssid' placeholder='ssid'></td></tr>"
          "<tr><td><input type='password' name='pass' placeholder='password'></td></tr>"
          "<tr><td><input type='submit' value='Connect'></td></tr>"
        "<table>"
      "</form>"
    "</body>"
  "</html>");

  //Start an access point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AccessIP, AccessDG, AccessSN);
  WiFi.softAP("OpenThermostat");

  Screen.valueScreen("Connect to","OpenThermostat");

  //Draw the login page
  webServer.on("/", [=](){
    webServer.send(200, "text/html", HTML);
    Screen.loadScreen("Device connected");
  });

  //When wifi credentials are submitted
  webServer.begin();
  webServer.on("/submit", [=](){
    submitForm();
  });

  accesPointActive = true;
}

void OpenThermostat::runAP()
{
  //If a device is connected show the IP address
  if (wifi_softap_get_station_num() > 0) {
      Screen.valueScreen("Visit","10.10.10.1");
  }

  webServer.handleClient();
}

void OpenThermostat::submitForm() {
  if (webServer.args() > 0 )
  {
    WiFi.begin(webServer.arg(0).c_str(), webServer.arg(1).c_str());
    accesPointActive = false;
    connectWIFI();
  }
}

//Get the wifi strength and draw the corresponding icon
void OpenThermostat::getWifiStrength()
{
  if ((millis() - lastWifiStrengthRead) > wifiStrengthReadInterval && Screen.activeScreen == HOME_SCREEN) {
    uint8_t strength = map(WiFi.RSSI(),-80,-67,1,3);
    strength = constrain(strength,1,3);

    Screen.sidebarIcons[0] = strength; //Set the corresponding wifi icon
    Screen.drawSidebar();

    lastWifiStrengthRead = millis();
  }
}

//Reads the current temperature and prints it to the home screen
void OpenThermostat::readTemperature()
{
  //Only read the temperature every 10 seconds and user is not setting the target temperature
  if ((millis() - lastTemperatureRead) > temperatureReadInterval && (millis() - lasttargetTemperatureRead) > targetTemperatureInterval)
  {
    float _temperature = Dht.readTemperature(tempMode);

    //Only save the temperature if it read correctly
    if (!isnan(_temperature)) {
      temperature = _temperature;
      temperature += tempCorrection;
    }

    //Only draw the temperature if the home screen is active
    if (Screen.activeScreen == HOME_SCREEN) {
      Screen.homeScreen(temperature);
      Screen.removeSidebarIcon(THERMOMETER_ICON);
    }

    //Add the current temperature to the average temperature
    if ((millis() - lastTemperatureAvg) > temperatureAvgInterval) {
      addAvgTemperature(temperature);
      lastTemperatureAvg = millis();
    }

    lastTemperatureRead = millis();
  }
}

void OpenThermostat::readRotary()
{
  if (rotaryValue == rotaryValueOld) return;

  switch (Screen.activeScreen) {

    case HOME_SCREEN:
    {
      float change = map((millis()-lasttargetTemperatureRead),0,100,5,1);
      change = constrain(change,1,10);
      change /= 10;
      if (rotaryValue > rotaryValueOld) {
        targetTemp += change;
      }
      else if (rotaryValue < rotaryValueOld) {
        targetTemp -= change;
      }
      targetTemp = constrain(targetTemp,0,30);
      lastTemperatureRead = 0; //Forces a temperature redraw after 2.5 seconds of turning
      lasttargetTemperatureRead = millis();
      Screen.addSidebarIcon(THERMOMETER_ICON);
      Screen.homeScreen(targetTemp);
      break;
    }

    case MENU_SCREEN:
    {
      if (rotaryValue > rotaryValueOld) {
        activeMenu+=1;
      } else if (rotaryValue < rotaryValueOld) {
        activeMenu-=1;
      }

      //Wrap the cursor if it goes out of bounds
      if (activeMenu >= Screen.menuLength) activeMenu = 0;
      else if (activeMenu < 0) activeMenu = (Screen.menuLength-1);

      Screen.menuScreen(activeMenu);
      break;
    }
  }
  rotaryValueOld = rotaryValue;
}

void OpenThermostat::readButton()
{
  int reading = analogRead(BUT_PIN);
  delay(3); //Delay needed to keep wifi connected

  if ((millis() - lastButtonRead) > buttonReadInterval && previous < 20 && reading > 200) {
    lastButtonRead = millis();

    //Check the current screen
    switch (Screen.activeScreen) {
      case HOME_SCREEN:
      Screen.menuScreen(0);
      break;

      case MENU_SCREEN:
      //Check the current main menu item
      switch (Screen.activeMenu) {
        case MAIN_MENU_RETURN:
          Screen.homeScreen(temperature);
          break;
        case MAIN_MENU_UPDATES:
          break;
        case MAIN_MENU_ID:
          Screen.valueScreen("ID Code",idCode);
          break;
        case MAIN_MENU_UNIT:
          char *unitType;
          if (tempMode == 0) unitType = "C";
          else if (tempMode == 1) unitType = "F";
          Screen.valueScreen("Unit",unitType);
          break;
        case MAIN_MENU_VERSION:
          Screen.valueScreen("Version",version);
          break;
      }
      break;

      case VALUE_SCREEN:
      Screen.menuScreen(activeMenu);
      break;
    }
  }
  previous = reading;
}

void OpenThermostat::PinA()
{
  detachInterrupt(ROTA_PIN);
  if (bitRead(gpio_input_get(), ROTB_PIN) == 1 && aFlag) {
    rotaryValue++;
    bFlag = 0;
    aFlag = 0;
  }
  else if (bitRead(gpio_input_get(), ROTB_PIN) == 0) bFlag = 1;
  attachInterrupt(ROTA_PIN, PinA, RISING);
}

void OpenThermostat::PinB()
{
  detachInterrupt(ROTB_PIN);
  if (bitRead(gpio_input_get(), ROTA_PIN) == 1 && bFlag) {
    rotaryValue--;
    bFlag = 0;
    aFlag = 0;
  }
  else if (bitRead(gpio_input_get(), ROTA_PIN) == 0) aFlag = 1;
  attachInterrupt(ROTB_PIN, PinB, RISING);
}

void OpenThermostat::EEPROM_writeID(int adress, char Str[])
{
  for (int i = 0; i < 9; i++)
  {
    EEPROM.write(adress, Str[i]);
    adress++;
  }
}

void OpenThermostat::EEPROM_readID(int adress)
{
  for (int i = 0; i < 9; i++)
  {
    char Character = EEPROM.read(adress);
    adress++;
    idCode[i] = Character;
  }
}

void OpenThermostat::addAvgTemperature(float _temperature)
{
  if (tempMode) {
    _temperature = ((_temperature - 32) * 5)/9;
  }

  for (uint8_t i = 0; i < 15; i++)
  {
    if (temperatureArray[i] == 0) {
      temperatureArray[i] = _temperature;
      return;
    }
  }
  for (uint8_t i = 0; i < 14; i++)
  {
    temperatureArray[i] = temperatureArray[i+1];
  }
  temperatureArray[14] = _temperature;
}

float OpenThermostat::getAvgTemperature()
{
  uint8_t count = 0;
  float totalTemperature = 0;
  for (uint8_t i = 0; i < 15; i++)
  {
    if (temperatureArray[i] != 0) {
      totalTemperature += temperatureArray[i];
      count++;
    }
  }
  return (totalTemperature/count);
}

void OpenThermostat::postTemperatureAvg()
{
  if ((millis() - lastTemperaturePost) > temperaturePostInterval && int(getAvgTemperature()) != 0) {
      postData(TEMPERATURE_POST);

      lastTemperaturePost = millis();
    }
}

void OpenThermostat::postTemperature()
{
  if ((millis() - lastTemperatureGet) > temperatureGetInterval && temperature != 0) {
      getData(GET_TEMPERATURE);

      lastTemperatureGet = millis();
    }
}

void OpenThermostat::getSettings()
{
  if ((millis() - lastSettingsGet) > settingsGetInterval) {
      getData(GET_SETTINGS);

      lastSettingsGet = millis();
    }
}

void OpenThermostat::postData(uint8_t type)
{
  WiFiClientSecure client;

  if (!client.connect(host, httpsPort)) {
    return;
  }

  if (!client.verify(fingerprint, host)) {
    return;
  }

  String url = "/api/";
  String data;

  switch(type) {
    case TEMPERATURE_POST:
      url += "post_data.php";

      data += "thermostat_identifier=";
      data += idCode;

      data += "&temperature=";
      data += getAvgTemperature();
      break;
  }

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Cache-Control: no-cache\r\n" +
               "Connection: close\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + data.length() + "\r\n\r\n" +
               ""+data+"\r\n");
}

void OpenThermostat::getData(uint8_t type)
{
  StaticJsonBuffer<200> jsonBuffer;
  WiFiClientSecure client;

  if (!client.connect(host, httpsPort)) {
    return;
  }

  if (!client.verify(fingerprint, host)) {
    return;
  }

  String url = "/api/";

  switch(type) {
    case GET_TEMPERATURE:
      url += "get_data?data=temperature";
      url += "&temperature=";
      url += temperature;
      url += "&thermostat_target=";
      url += targetTemp;
      url += "&thermostat_identifier=";
      url += idCode;
      break;
    case GET_STARTUP_SETTINGS:
      url += "get_data?data=startup_settings";
      url += "&thermostat_identifier=";
      url += idCode;
      break;
    case GET_SETTINGS:
      url += "get_data?data=settings";
      url += "&thermostat_identifier=";
      url += idCode;
      break;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  //Read the response from the server
  while (client.connected()) {
    String line = client.readStringUntil('\n');

    //If the json line is found
    if (line.startsWith("{")) {
      Serial.println(line);

      const char *json = line.c_str();

      JsonObject& jsonResponse = jsonBuffer.parseObject(json);

      // Test if parsing succeeds
      if (!jsonResponse.success()) {
        Serial.println("JSON Parsing failed");
        return;
      }

      switch(type) {
        case GET_TEMPERATURE:
          targetTempWeb = jsonResponse["web_target"];
          if (targetTempWeb != targetTempWebOld) {
            targetTemp = targetTempWebOld = targetTempWeb;
          }
          break;
        case GET_STARTUP_SETTINGS:
          targetTemp = targetTempWeb = targetTempWebOld = jsonResponse["web_target"];
          tempCorrection  = jsonResponse["temp_correction"];
          tempMode        = jsonResponse["unit"];
          break;
        case GET_SETTINGS:
          tempCorrection  = jsonResponse["temp_correction"];
          tempMode        = jsonResponse["unit"];
          updateAvailable = jsonResponse["update_available"];

          if (updateAvailable == 1) {
            Screen.addSidebarIcon(UPDATE_ICON);
            Serial.println("Update");
          }
          break;
      }
    }
  }
}
