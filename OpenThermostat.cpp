#include "OpenThermostat.h";

OpenThermostat::OpenThermostat()
{
  tempCorrection = -3;
  tempMode = CELCIUS;
  targetTemp = 0;
  aFlag = 0;
  bFlag = 0;
  encoderPos = 0;
  oldEncPos = 0;
  buttonPressed = false;
}

void OpenThermostat::begin()
{
  Screen.begin();
  Dht.begin();

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
   delay(2000);
   Screen.addSidebarIcon(HEATING_ICON);
   delay(2000);
   Screen.addSidebarIcon(HEATING_ICON);
   delay(2000);
   Screen.addSidebarIcon(UPDATE_ICON);
   delay(2000);
   Screen.removeSidebarIcon(HEATING_ICON);
   delay(2000);
   Screen.removeSidebarIcon(UPDATE_ICON);
   delay(2000);
   Screen.addSidebarIcon(UPDATE_ICON);
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
      Screen.loadScreen("Connected!");
      delay(1300);
      break;

    //If unable to connect start an access point
    case WL_NO_SSID_AVAIL:
      setupAP();

      //Loop the access point to poll for user input
      while(accesPointActive) runAP();
      break;
  }
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

  webServer.on("/", [=](){
    webServer.send(200, "text/html", HTML);
    Screen.loadScreen("Device connected");
  });
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
    Serial.println(webServer.arg(0));
    Serial.println(webServer.arg(1));

    WiFi.begin(webServer.arg(0).c_str(), webServer.arg(1).c_str());
    accesPointActive = false;
    connectWIFI();
  }
}

//Get the wifi strength and draw the corresponding icon
void OpenThermostat::getWifiStrength()
{
  if (Screen.activeScreen == HOME_SCREEN) {
    uint8_t strength = map(WiFi.RSSI(),-80,-67,1,3);
    strength = constrain(strength,1,3);

    Screen.sidebarIcons[0] = strength; //Set the corresponding wifi icon
    Screen.drawSidebar();
  }
}

//Reads the current temperature and prints it to the home screen
void OpenThermostat::readTemperature()
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
  }
}

void OpenThermostat::PinA()
{
  detachInterrupt(ROTA_PIN);
  readingA = digitalRead(ROTA_PIN);
  readingB = digitalRead(ROTB_PIN);
  if (readingA == HIGH && readingB == HIGH && aFlag) {
    targetTemp+=0.1;
    //do something here
    bFlag = 0;
    aFlag = 0;
  }
  else if (readingA == HIGH && readingB == LOW) bFlag = 1;
  attachInterrupt(ROTA_PIN, PinA, RISING);
}

void OpenThermostat::PinB()
{
  detachInterrupt(ROTB_PIN);
  readingA = digitalRead(ROTA_PIN);
  readingB = digitalRead(ROTB_PIN);
  if (readingA == HIGH && readingB == HIGH && bFlag) {
    ra==targetTemp-=0.1;
    //do something here
    bFlag = 0;
    aFlag = 0;
  }
  else if (readingB == HIGH && readingA == LOW) aFlag = 1;
  attachInterrupt(ROTB_PIN, PinB, RISING);
}

void OpenThermostat::readButton()
{
  if(analogRead(BUT_PIN) > 20) {
    buttonPressed = true;
  }
}
