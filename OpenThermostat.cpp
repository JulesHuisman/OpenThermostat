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

  previous = 0;

  wifiStrengthReadInterval         = 60000; //How often to read wifi strength (60 s)
  temperatureReadInterval          = 10000; //How often to get the indoor temperature (10 s)
  targetTemperatureReadInterval    = 2500; //How long to display the set temperature (2.5 s)
  menuInterval                     = 5000; //How long to display the menu (5 s)
  temperaturePostInterval          = 900000; //The time between temperature posts (15 min)
  temperatureAvgInterval           = 60000; //The time between adding to average temperature (1 min)
  scheduleGetInterval              = 60000; //How often to post and get the temperature and target temperature (1 min)
  settingsGetInterval              = 300000; //How often toget the current settings and updates (5 min)
  buttonReadInterval               = 150; //Debounce delay for readButton() (150 ms)
  heatingInterval                  = 60000; //Minumum interval required between heating (1 min) (gets adapted when getting settings)
  heatingCheckInterval             = 10000; //How often to check the heating status

  lastWifiStrengthRead = -wifiStrengthReadInterval; //Forces get wifi strength on startup
  lastTemperatureRead  = -temperatureReadInterval; //Forces get temperature on startup
  lastTemperaturePost  = -temperaturePostInterval; //Forces posting the temperature on startup
  lastTemperatureAvg   = -temperatureAvgInterval; //Forces adding to average temperature on startup
  lastHeating          = -heatingInterval; //Forces checking the schedule on startup
}

void OpenThermostat::begin()
{
  Screen.begin();
  Dht.begin();
  EEPROM.begin(16);

  Serial.setDebugOutput(true);

  //Get the ID code for this thermostat and save it in a variable
  EEPROM_readID();

  //PinModes and intterupts for the rotary encoder
  pinMode(ROTA_PIN, INPUT_PULLUP);
  pinMode(ROTB_PIN, INPUT_PULLUP);
  pinMode(HEATING_PIN,OUTPUT);
  attachInterrupt(ROTA_PIN, PinA, RISING);
  attachInterrupt(ROTB_PIN, PinB, RISING);

  digitalWrite(HEATING_PIN,LOW);

  connectWIFI();

  Screen.homeScreen(0,0);

}

//The loop function of the library
void OpenThermostat::run()
{
  Serial.println(Screen.activeScreen);
  getWifiStrength();
  readTemperature();
  if(offlineMode == false)
  {
    postTemperatureAvg();
    getSchedule();
    getSettings();
  }
  checkTargetTemperature();
  readRotary();
  readButton();
  checkHeating();

  //Returns to showing the homeScreen after inactivity in the menu.
  if(Screen.activeScreen == MENU_SCREEN && (millis() - lastMenuRead) > menuInterval)
  {
   Screen.homeScreen(temperature, targetTemperature);
 } else if (Screen.activeScreen == VALUE_SCREEN && (millis() - lastMenuRead) > menuInterval)
 {
   Screen.homeScreen(temperature, targetTemperature);
 }
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

    //If unable to connect choose between offline mode or start an access point
    case WL_NO_SSID_AVAIL:

      offlineMode = true;
      Screen.offlineModeOption = true;
      //Draw menu with offlineMode option
      Screen.menuScreen(0);
      //Wait for the user to choose an option
      while(buttonClicked == false) {
        readButton();
        readRotary();
      }

      if(offlineMode == false)
      {
        setupAP();
        //Loop the access point to poll for user input
        while(accesPointActive) runAP();
        break;

      } else if(offlineMode == true){
        Screen.addSidebarIcon(NO_INTERNET_ICON);
    }
  }
}

void OpenThermostat::getStartupSettings()
{
  getData(GET_SETTINGS);
  delay(5);
  getData(GET_SCHEDULE);
  Screen.homeScreen(temperature,targetTemperature);
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
  if ((millis() - lastWifiStrengthRead) > wifiStrengthReadInterval && Screen.activeScreen == HOME_SCREEN && targetTemperatureChanged == false) {
    if(WiFi.status() == WL_CONNECTED){
    uint8_t strength = map(WiFi.RSSI(),-80,-67,1,3);
    strength = constrain(strength,1,3);

    Screen.sidebarIcons[0] = strength; //Set the corresponding wifi icon
  } else {
    Screen.sidebarIcons[0] = NO_INTERNET_ICON;
  }
    Screen.drawSidebar();

    lastWifiStrengthRead = millis();
  }
}

//Reads the current temperature and prints it to the home screen
void OpenThermostat::readTemperature()
{
  if ((millis() - lastTemperatureRead) > temperatureReadInterval && targetTemperatureChanged == false)
  {
    float _temperature = Dht.readTemperature(tempMode);

    //Only save the temperature if it read correctly
    if (!isnan(_temperature)) {
      temperature = _temperature;
      temperature += tempCorrection;
    }

    //Only draw the temperature if the home screen is active
    if (Screen.activeScreen == HOME_SCREEN) {
      Screen.homeScreen(temperature, targetTemperature);
      Screen.removeSidebarIcon(THERMOMETER_ICON);
    }

    //Add the current temperature to the average temperature
    if ((millis() - lastTemperatureAvg) > temperatureAvgInterval) {
      addAvgTemperature(temperature);
      lastTemperatureAvg = millis();
    }
  }
}

//Checks if the temperature has been set
void OpenThermostat::checkTargetTemperature()
{
  if ((millis() - lastTargetTemperatureRead) > targetTemperatureReadInterval && targetTemperatureChanged == true)
  {
    Serial.println("Target Changed");
    targetTemperatureChanged = false;
  }
}

//Check if the rotary has been turned
void OpenThermostat::readRotary()
{
  //If the rotary has not been turned, return
  if (rotaryValue == rotaryValueOld) return;

  //Check which screen is active
  switch (Screen.activeScreen) {

    //If the home screen is active change the target temperature
    case HOME_SCREEN:
    {
      //Change the target temperature quicker if the rotary is turned faster
      float change = map((millis()-lastTargetTemperatureRead),0,100,3,1);
      change = constrain(change,1,3);
      change /= 10;

      if (rotaryValue > rotaryValueOld)      targetTemperature += change;
      else if (rotaryValue < rotaryValueOld) targetTemperature -= change;

      targetTemperature = constrain(targetTemperature,0,30); //TODO change for fahrenheit

      lastTemperatureRead = 0; //Forces a temperature redraw after 2.5 seconds of turning the rotary
      lastTargetTemperatureRead = millis();
      targetTemperatureChanged = true;

      Screen.homeScreen(temperature, targetTemperature);
      break;
    }

    //If the menu is active scroll through the items
    case MENU_SCREEN:
    {
      if (rotaryValue > rotaryValueOld) {
        activeMenu+=1;
      } else if (rotaryValue < rotaryValueOld) {
        activeMenu-=1;
      }

      if(Screen.offlineModeOption == false){
        Screen.menuLength = MAIN_MENU_LENGTH;
      } else {
        Screen.menuLength = OFFLINE_MODE_MENU_LENGTH;
      }

      //Wrap the cursor if it goes out of bounds
      if (activeMenu >= Screen.menuLength) activeMenu = 0;
      else if (activeMenu < 0) activeMenu = (Screen.menuLength-1);

      lastMenuRead = millis();
      Screen.menuScreen(activeMenu);
      break;
    }
  }
  rotaryValueOld = rotaryValue;
}

//Check if the rotary button has been pressed
void OpenThermostat::readButton()
{
  int reading = analogRead(BUT_PIN);
  delay(3); //Delay needed to keep wifi connected (esp8266 bug)

  if ((millis() - lastButtonRead) > buttonReadInterval && previous < 20 && reading > 200) {
    lastButtonRead = millis();

    //Check the current screen
    switch (Screen.activeScreen) {
      case HOME_SCREEN:
      lastMenuRead = millis();
      Screen.menuScreen(0);
      break;

      case MENU_SCREEN:
      //Check the current main menu item
      if(Screen.offlineModeOption == false) {
        switch (Screen.activeMenu) {
          case MAIN_MENU_RETURN:
            Screen.homeScreen(temperature, targetTemperature);
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
      } else {
        switch (Screen.activeMenu) {
          case OFFLINE_MODE:
            //Disable the offlineModeOption menu items
            Screen.offlineModeOption = false;
          break;
          case ACCES_POINT:
            offlineMode = false;
          break;
        }
      }
      lastMenuRead = millis();
      break;

      case VALUE_SCREEN:
        Screen.menuScreen(activeMenu);
        lastMenuRead = millis();
      break;
    }
    buttonClicked = true;
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

void OpenThermostat::EEPROM_readID()
{
  for (int i = 0; i < 16; i++)
  {
    char Character = EEPROM.read(i);
    idCode[i] = Character;
  }
}

void OpenThermostat::addAvgTemperature(float _temperature)
{
  //Convert to fahrenheit
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

//Check the current schedule and checks if it needs to start heating
void OpenThermostat::checkHeating() {
  if ((millis() - lastHeatingCheck) > heatingCheckInterval) {
    bool scheduleActive = false;

    for (uint8_t i = 0; i < scheduleLength; i++) {
      if (time >= schedule[i][0] && time < schedule[i][1]) {

        targetTemperature = schedule[i][2];
        scheduleActive = true;
        break;
      }
    }

    //If there is no active module default to resting temperature
    if (!scheduleActive) {
      targetTemperature = restingTemp;
    }

    //If the temperature dips below the target temperature
    if (targetTemperature > temperature && temperature != 0) {
      heatingOn(true);
    } else {
      heatingOn(false);
    }

    //If the current temperature is lower that the old dip temperature
    if (temperature < dipTemperature) {
      dipTemperature = temperature;
      dipTime        = millis();
    }

    lastHeatingCheck = millis();
  }
}

void OpenThermostat::heatingOn(bool _heating) {
  if (targetTemperatureChanged == false)
  {
    if (_heating && ((millis() - lastHeating) > heatingInterval) && heating == false)
    {
      Serial.println("Starting heating");
      Screen.addSidebarIcon(HEATING_ICON);
      digitalWrite(HEATING_PIN,HIGH);

      lastHeating      = millis(); //To check the heating interval
      heatingStartTime = millis(); //To calculate the duration of the heating session
      dipTime          = millis(); //Set the time of the heating dip to the current time

      startTemperature = temperature;
      dipTemperature   = temperature;

      heating = true;
    }
    else if (!_heating && heating == true) {
      Serial.println("Stopped heating");
      Screen.removeSidebarIcon(HEATING_ICON);
      digitalWrite(HEATING_PIN,LOW);

      //When a heating session is done, set a flag to send the heating data to the dashboard
      postData(HEATING_POST);
      heating = false;
    }
  }
}

void OpenThermostat::postTemperatureAvg()
{
  if ((millis() - lastTemperaturePost) > temperaturePostInterval && int(getAvgTemperature()) != 0 && temperature != 0 && targetTemperatureChanged == false) {
      postData(TEMPERATURE_POST);

      lastTemperaturePost = millis();
    }
}

//Posts the current indoor temperature, server returns schedule if available
void OpenThermostat::getSchedule()
{
  if ((millis() - lastScheduleGet) > scheduleGetInterval && temperature != 0 && targetTemperatureChanged == false) {
      getData(GET_SCHEDULE);

      lastScheduleGet = millis();
    }
}

void OpenThermostat::getSettings()
{
  if ((millis() - lastSettingsGet) > settingsGetInterval && targetTemperatureChanged == false) {
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
      url += "post_data";

      data += "thermostat_identifier=";
      data += idCode;
      data += "&temperature=";
      data += getAvgTemperature();
      data += "&data=";
      data += "temperature";
      break;
    case HEATING_POST:
      url += "post_data";

      data += "thermostat_identifier=";
      data += idCode;
      data += "&startTemperature=";
      data += startTemperature;
      data += "&dipTemperature=";
      data += dipTemperature;
      data += "&targetTemperature=";
      data += targetTemperature;
      data += "&heatingTime=";
      data += (millis()-heatingStartTime)/1000;
      data += "&dipTime=";
      data += (dipTime-heatingStartTime)/1000;
      data += "&data=";
      data += "heating";
      break;
  }

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Cache-Control: no-cache\r\n" +
               "Connection: close\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + data.length() + "\r\n\r\n" +
               ""+data+"\r\n");

   while (client.connected()) {
     String line = client.readStringUntil('\n');

      Serial.println(line);
   }
}

void OpenThermostat::getData(uint8_t type)
{
  StaticJsonBuffer<1024> jsonBuffer;
  WiFiClientSecure client;

  if (!client.connect(host, httpsPort)) {
    return;
  }

  if (!client.verify(fingerprint, host)) {
    return;
  }

  String url = "/api/";

  switch(type) {
    case GET_SCHEDULE:
      url += "get_data?data=schedule";
      url += "&temperature=";
      url += temperature;
      url += "&thermostat_target=";
      url += targetTemperature;
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

      JsonObject& jsonResponse = jsonBuffer.parseObject(json, 2);

      // Test if parsing succeeds
      if (!jsonResponse.success()) {
        Serial.println("JSON Parsing failed");
        return;
      }

      int scheduleAvailable;
      scheduleLength;

      switch(type) {
        case GET_SCHEDULE:
          scheduleAvailable = jsonResponse["available"];
          time              = jsonResponse["time"];

          if (scheduleAvailable == 1) {
            scheduleLength = jsonResponse["len"];

            for (uint8_t i = 0; i < scheduleLength; i++) {
              String key = "";
              key += String(i);
              JsonArray& module = jsonResponse[""+key+""];

              int start  = module[0];
              int end    = module[1];
              float temp = module[2];

              //Multiply by 15 to go from quarters to minutes
              start *= 15;
              end   *= 15;
              end   += start;

              schedule[i][0] = start;
              schedule[i][1] = end;
              schedule[i][2] = temp;
            }
            lastHeatingCheck = 0; //Forces checking of the schedule
          }
          break;
        case GET_SETTINGS:
          tempCorrection  = jsonResponse["temp_correction"];
          tempMode        = jsonResponse["unit"];
          updateAvailable = jsonResponse["update_available"];
          restingTemp     = jsonResponse["rest_temperature"];
          heatingInterval = jsonResponse["heating_interval"];

          heatingInterval *= 60000;

          if (updateAvailable == 1) {
            Screen.addSidebarIcon(UPDATE_ICON);
          }
          break;
      }
    }
  }
}
