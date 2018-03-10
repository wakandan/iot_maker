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

// for PubSubClient for mqtt
#include <PubSubClient.h>

//information about mqtt cloud
#define mqtt_server "m14.cloudmqtt.com" // Thay bằng thông tin của bạn
#define mqtt_topic_pub "home/door"   //Giữ nguyên nếu bạn tạo topic tên là demo
#define mqtt_topic_sub "home/door"
#define mqtt_user "rmtdobup"    //Giữ nguyên nếu bạn tạo user là esp8266 và pass là 123456
#define mqtt_pwd "UV9IS3WZUHbm"
#define mqtt_port 17036
#define mqtt_ssl_port 27036


#define SUCCESS_CODE 200
#define CONTENT_TYPE "text/plain"
#define DEFAULT_RESPONSE "1"
#define DOOR_CODE_UP 3962048
#define DOOR_CODE_DOWN 3961859
#define PORT_DOOR_SEND D5
#define DOOR_CODE_BIT_SIZE 24

WiFiClient espClient;

PubSubClient mqtt_client(espClient);

ESP8266WebServer server(80);

RCSwitch rfDoorSendSwitch = RCSwitch();

void get_string(byte* payload, char* buffer, unsigned int length) {
  for (int i = 0; i < length; i++) {
    buffer[i] = payload[i];
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("message arrived topic=[%s]\n", topic);
  char msg[length+1];
  //get the message
  get_string(payload, msg, length);
  if (String(topic) == mqtt_topic_pub) {
    Serial.printf("got msg for door [%s]\n", msg);
    
    if(String(msg)=="1") { //open door
      Serial.println("open door up");
      order_door_up();
    } else {
      Serial.println("close door down");
      order_door_down();
    }
  }
}

void setup_mqtt(void) {
  mqtt_client.setServer(mqtt_server, mqtt_port); 
  mqtt_client.setCallback(mqtt_callback);
}

void reconnect_mqtt(void) {
  while (!mqtt_client.connected()) {
    Serial.println("attempting to connect to mqtt");
    if (mqtt_client.connect("Door client", mqtt_user, mqtt_pwd)) {
      Serial.println("mqtt connected");
      mqtt_client.publish(mqtt_topic_pub, "esp_connected");
      mqtt_client.subscribe(mqtt_topic_sub);
    } else {
      Serial.println("connecting to mqtt failed");
      Serial.printf("mqtt client state = %s\n", mqtt_client.state());
      Serial.println("will try again after 5 seconds");
      delay(5000);
    }
  }
}

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

void order_door_up() {
  rfDoorSendSwitch.send(DOOR_CODE_UP, DOOR_CODE_BIT_SIZE);
  delay(2000);
}

void order_door_down() {
  rfDoorSendSwitch.send(DOOR_CODE_DOWN, DOOR_CODE_BIT_SIZE);
  delay(2000);
}

void handle_door_up() {
  Serial.println("handle door up");
  order_door_up();
  server.send(SUCCESS_CODE, CONTENT_TYPE, DEFAULT_RESPONSE);
  Serial.println("handle door up done");
}

void handle_door_down() {
  Serial.println("handle door down");
  order_door_down();
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

  setup_mqtt();
}

void loop() {
  // put your main code here, to run repeatedly:
  //make sure mqtt is connected
  //trying to connect to mqtt
  if(!mqtt_client.connected()) {
    reconnect_mqtt();
  }
  mqtt_client.loop();

  //let the server handle client connections
  server.handleClient();
}
