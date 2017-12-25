#include "OpenThermostat.h"

long OpenThermostat::rotaryValue = 0;
volatile uint8_t OpenThermostat::aFlag = 0;
volatile uint8_t OpenThermostat::bFlag = 0;
volatile uint8_t OpenThermostat::encoderPos = 0;
volatile uint8_t OpenThermostat::oldEncPos = 0;
volatile uint8_t OpenThermostat::readingA;
volatile uint8_t OpenThermostat::readingB;

OpenThermostat::OpenThermostat()
{
  tempCorrection      = 0;
  unit                = CELCIUS;
  websocketAuthorized = true;
  //comment

  wifiStrengthTimer[READ_INTERVAL]      = 30000; //How often to read wifi strength (30 s)
  temperatureTimer[READ_INTERVAL]       = 10000; //How often to read the dht temperature (10 s)
  buttonTimer[READ_INTERVAL]            = 150;   //Button debounce interval (0.15 s)

  forceTimer(wifiStrengthTimer); //Forces get wifi strength on startup
  forceTimer(temperatureTimer);  //Forces get temperature on startup
}

/**
 * The function that is called when the thermostat starts
 * It sets up all the connections and pins
 *
 * @return void
 */
void OpenThermostat::begin()
{
  Screen.begin();
  Dht.begin();
  EEPROM.begin(16);

  connectWIFI();

	// webSocket.beginSSL("dashboard.open-thermostat.com", 443, "/wss/", "", "");
	webSocket.begin("192.168.0.107", 8080, "/", "");
    webSocket.onEvent(std::bind(&OpenThermostat::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	webSocket.setReconnectInterval(5000);

  Serial.setDebugOutput(false); //Set wifi debugging

  //PinModes and intterupts for the rotary encoder
  pinMode(ROTA_PIN, INPUT_PULLUP);
  pinMode(ROTB_PIN, INPUT_PULLUP);
  pinMode(HEATING_PIN,OUTPUT);
  attachInterrupt(ROTA_PIN, PinA, RISING);
  attachInterrupt(ROTB_PIN, PinB, RISING);
  digitalWrite(HEATING_PIN,LOW);

  Screen.homeScreen(0,0);

  //Generate the websocket token
  keygen();
}

/**
 * The loop function of the library
 *
 * @return void
 */
void OpenThermostat::run()
{
  getWifiStrength();
  readTemperature();
  readRotary();
  readButton();

  //Handle websocket connection
  if (websocketAuthorized) webSocket.loop();

  yield();
}

/**
 * Connect the esp8266 to the wifi the user entered in the access point
 *
 * @return void
 */
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

    //Stop loading when connected
    if (WiFi.status() == WL_CONNECTED) break;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Screen.addSidebarIcon(NO_INTERNET_ICON);
    Screen.drawSidebar();
  }
}

/**
 * Get the active SSID name and convert it to a character array
 *
 * @return void
 */
void OpenThermostat::getSSID()
{
  String ssid = WiFi.SSID();
  strcpy(SSID, ssid.c_str());
}

/**
 * Sets up the esp8266 as an access point so the user can enter their wifi credentials
 *
 * @return void
 */
void OpenThermostat::setupAP()
{
  IPAddress AccessIP = IPAddress(10, 10, 10, 1);
  IPAddress AccessDG = IPAddress(10, 10, 10, 1);
  IPAddress AccessSN = IPAddress(255, 255, 255, 0);

  const byte DNS_PORT = 53;

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

  dnsServer.start(DNS_PORT, "*", AccessIP);

  Screen.valueScreen("Connect to","OpenThermostat");

  //Draw the login page
  webServer.onNotFound([=](){
    webServer.send(200, "text/html", HTML);
    Screen.loadScreen("Device connected");
  });

  //When wifi credentials are submitted
  webServer.begin();
  webServer.on("/submit", [=](){
    submitForm();
  });
}

/**
 * Handles the access points events
 *
 * @return void
 */
void OpenThermostat::runAP()
{
  dnsServer.processNextRequest();
  webServer.handleClient();
}

/**
 * If the form from the access point is submitted this function is called
 *
 * @return void
 */
void OpenThermostat::submitForm()
{
  if (webServer.args() > 0 )
  {
    WiFi.begin(webServer.arg(0).c_str(), webServer.arg(1).c_str());
    connectWIFI();
  }
}

/**
 * Get the wifi strength and draw the corresponding icon
 *
 * @return void
 */
void OpenThermostat::getWifiStrength()
{
  if (timerReady(wifiStrengthTimer) && Screen.activeScreen == HOME_SCREEN) {
    uint8_t strength = map(WiFi.RSSI(),-80,-67,1,3);
    strength = constrain(strength,1,3);

    //Set the corresponding wifi icon
    Screen.sidebarIcons[0] = strength;
    Screen.drawSidebar();
  }
}

/**
 * Reads the current temperature and prints it to the home screen
 *
 * @return void
 */
void OpenThermostat::readTemperature()
{
  //Don't read the temperature when the rotary is turning, as the temperature reading is blocking
  if (rotaryTurning) return;

  if (timerReady(temperatureTimer))
  {
    float _temperature = Dht.readTemperature();

    //Only save the temperature if it read correctly
    if (!isnan(_temperature)) {
      //Successfully read temperature
      temperature = _temperature;
      temperature += tempCorrection;
    } else {
      //Refresh faster when failing to read temperature
      temperatureTimer[LAST_READ] = millis() + 500;
    }

    //Only draw the temperature if the home screen is active
    if (Screen.activeScreen == HOME_SCREEN) {
      Screen.homeScreen(temperature, targetTemperature);
    }
  }
}

/**
 * Check if the rotary has been turned
 * If the rotary was turned check which screen is active
 *
 * @return void
 */
void OpenThermostat::readRotary()
{
  //If the rotary has not been turned, return
  if (rotaryValue == rotaryValueOld) {
    //Set rotaryTurning to false when it was more than 1000ms ago that the rotary was turned
    if (millis() - rotaryTimer[LAST_READ] > 1000 && rotaryTurning) rotaryTurning = false;
    return;
  }

  rotaryTurning = true;

  //Check which screen is active
  switch (Screen.activeScreen)
  {
    //If the home screen is active change the target temperature
    case HOME_SCREEN:
    {
      //Change the target temperature quicker if the rotary is turned faster
      float change = map((millis()-rotaryTimer[LAST_READ]),0,100,3,1);
      change = constrain(change,1,3);
      change /= 10;

      if (rotaryValue > rotaryValueOld)      targetTemperature += change;
      else if (rotaryValue < rotaryValueOld) targetTemperature -= change;

      targetTemperature = constrain(targetTemperature,0,30);

      Screen.homeScreen(temperature, targetTemperature);

      rotaryTimer[LAST_READ] = millis();
      break;
    }

    //If the menu is active scroll through the items
    case MENU_SCREEN:
    {
      int activeMenuItem;

      if (rotaryValue > rotaryValueOld) {
        activeMenuItem += 1;
      } else if (rotaryValue < rotaryValueOld) {
        activeMenuItem -= 1;
      }

      Screen.menuLength = MAIN_MENU_LENGTH;

      //Wrap the cursor if it goes out of bounds
      if (activeMenuItem >= Screen.menuLength) activeMenuItem = 0;
      else if (activeMenuItem < 0)             activeMenuItem = (Screen.menuLength-1);

      Screen.menuScreen(activeMenuItem);
      break;
    }
  }
  rotaryValueOld = rotaryValue;
}

/**
 * Check if the rotary button has been pressed
 * If the button is pressed check which screen is active
 *
 * @return void
 */
void OpenThermostat::readButton()
{
  unsigned int buttonValue[2];

  buttonValue[LAST]    = 0;
  buttonValue[CURRENT] = analogRead(BUT_PIN);
  delay(3); //Delay needed to keep wifi connected (esp8266 bug)

  //If the button has been pressed, check the active screen
  if (timerReady(buttonTimer) && buttonValue[LAST] < 20 && buttonValue[CURRENT] > 200) {

    //Check the current screen
    switch (Screen.activeScreen)
    {
      //If the home screen is active open the menu on button click
      case HOME_SCREEN:

        Screen.menuScreen(0);
        break;

      //Check the current main menu item
      case MENU_SCREEN:

        switch (Screen.activeMenu)
        {
          //The return button
          case MAIN_MENU_RETURN:
            Screen.homeScreen(temperature, targetTemperature);
            break;

          //The update button
          case MAIN_MENU_UPDATE:
            //TODO: Update code here
            break;
          }

      case VALUE_SCREEN:
        Screen.menuScreen(0);
      break;
    }
  }

  buttonValue[LAST] = buttonValue[CURRENT];
}

/**
 * The function the interupt calls when rotary pin A changes
 *
 * @return void
 */
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

/**
 * The function the interupt calls when rotary pin B changes
 *
 * @return void
 */
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

/**
 * Generate the authentication key
 *
 * @return void
 */
void OpenThermostat::keygen()
{
    BYTE hash[SHA256_BLOCK_SIZE];
    char texthash[2*SHA256_BLOCK_SIZE+1];

    Sha256* test=new Sha256();
    BYTE text[] = "administrator@email.com:6hk7jgivn3gfghrydjkuld7vjhjdurg6:5kj9dfk3lf8dsj3kjdf";
    test->update(text, strlen((const char*)text));
    test->final(hash);

    for(int i=0; i<SHA256_BLOCK_SIZE; ++i)
      sprintf(texthash+2*i, "%02X", hash[i]);

    Serial.println();
    Serial.print("Hash: ");
    Serial.println(texthash);
}

/**
 * The websocket event watcher
 *
 * @param (WStype_t type)     -> The type of data the socket receives
 * @param (uint8_t * payload) -> The content of the data
 * @param (size_t length)     -> The length of the data
 *
 * @return void
 */
void OpenThermostat::webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
	switch(type) {

		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");
      Screen.removeSidebarIcon(SOCKET_ICON);
      Screen.drawSidebar();
			break;

		case WStype_CONNECTED: {
			Serial.printf("[WSc] Connected to url: %s\n", payload);
      if (websocketAuthorized) connectWebsocket();
		}
			break;

		case WStype_TEXT:
			Serial.printf("[WSc] get text: %s\n", payload);
      handlePayload(payload);
			break;

		case WStype_BIN:
			Serial.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);
			break;

	}
}

void OpenThermostat::connectWebsocket()
{
  Serial.println("Connecting websockets");
  //Create the json object
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["type"]       = "thermostat";
  json["identifier"] = "administrator@email.com";
  json["key"]        = "6cdf285f3fedd95ae9ec6365f535e22bbe107feccc30cfaff24f97cda0fb8b81";

  String payload;
  json.printTo(payload);

  webSocket.sendTXT(payload);
}

/**
 * This function handles the incoming payloads from the websocket connection
 *
 * @param (uint8_t *payload) -> The payload of the websocket message

 * @return void
 */
void OpenThermostat::handlePayload(uint8_t *payload)
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(payload);

  //Check if the payload is the authorization message
  if (json.containsKey("authorized")) {
     websocketAuthorized = json["authorized"];

     //If authorized add the socket icon
     if (websocketAuthorized) Screen.addSidebarIcon(SOCKET_ICON);

     return;
  }
}

/**
 * Check if a timer is ready to fire again.
 *
 * @param (unsigned long timer[]) -> The timer to check

 * @return bool
 */
bool OpenThermostat::timerReady(unsigned long timer[])
{
  if ((millis() - timer[LAST_READ]) > timer[READ_INTERVAL]) {
    timer[LAST_READ] = millis();
    return true;
  }
  else {
    return false;
  }
}

/**
 * Force a timer to fire again by setting the last read to a negative interval
 *
 * @param (unsigned long timer[]) -> The timer to reset
 *
 * @return void
 */
void OpenThermostat::forceTimer(unsigned long timer[])
{
  timer[LAST_READ] = -timer[READ_INTERVAL];
}
