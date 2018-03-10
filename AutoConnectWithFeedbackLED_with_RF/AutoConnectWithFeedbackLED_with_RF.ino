#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

//for LED status
#include <Ticker.h>
Ticker ticker;

// for RC switch
#include <RCSwitch.h>

ESP8266WebServer server(80);

const int SUCCESS_CODE=200;
const char* CONTENT_TYPE="text/plain";
const char* DEFAULT_RESPONSE="1";
const int DOOR_CODE_UP=3962048;
const int DOOR_CODE_DOWN=3961859;
const int PORT_DOOR_SEND=D5;
const int DOOR_CODE_BIT_SIZE=24;

RCSwitch rfDoorSendSwitch = RCSwitch();

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void handle_door_up() {
  Serial.println("handle door up");
  rfDoorSendSwitch.send(DOOR_CODE_UP, DOOR_CODE_BIT_SIZE);
  delay(2000);
  server.send(SUCCESS_CODE, CONTENT_TYPE, DEFAULT_RESPONSE);
  Serial.println("handle door up done");
}

void handle_door_down() {
  Serial.println("handle door down");
  rfDoorSendSwitch.send(DOOR_CODE_DOWN, DOOR_CODE_BIT_SIZE);
  delay(2000);
  server.send(SUCCESS_CODE, CONTENT_TYPE, DEFAULT_RESPONSE);
  Serial.println("handle door down done");
}

void handle_not_found() {
  Serial.println("not found");
  server.send(SUCCESS_CODE, CONTENT_TYPE, "not found");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  //set up rc switch send
  rfDoorSendSwitch.enableTransmit(PORT_DOOR_SEND);

  Serial.println("starting wifi server");
  server.on("/door_up", handle_door_up);
  server.on("/door_down", handle_door_down);
  server.onNotFound(handle_not_found);
  server.begin();
  Serial.println("started wifi server");
}

void loop() {
  // put your main code here, to run repeatedly:

  //let the server handle client connections
  server.handleClient();
}
