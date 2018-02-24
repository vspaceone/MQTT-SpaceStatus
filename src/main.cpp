#include <FS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <cstring>

#include "config.h"
#include "neopixel.h"
#include "state.h"

#define interruptStatePin 14

WiFiClient espClient;
PubSubClient mqttClient(espClient);

State state;
NeopixelInterface interface(&state);

long lastCheck = 0;

bool stateOpen = false;

volatile byte interruptStateCounter = 1;

void connectWiFi() {
  delay(10);

  Serial.println("");

  WiFi.begin(wifi_ssid, wifi_password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    ESP.wdtFeed();
    Serial.print(".");
    interface.loop();
  }
  
  Serial.println("");
  Serial.print("WiFi connected ");
  Serial.println(WiFi.localIP());
  Serial.print("MQTT configured to ");
  Serial.print(mqtt_server);
  Serial.print(" with user ");
  Serial.println(mqtt_user);
}

void connectMQTT() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("vspace.one.state.open", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void checkMQTTConnectionAndLoop(){
  if (!mqttClient.connected()) {
    state.setConnectionState(ConnectionState::NO_MQTT);
    connectMQTT();
  } else if (state.getConnectionState() != ConnectionState::CONN_OK) {
    state.setConnectionState(ConnectionState::CONN_OK);
  }
  mqttClient.loop();
}

void requestSpaceStateUpdate(){
  //Serial.println("Requesting state");
  mqttClient.publish("vspace/one/request", "{\"status\":\"ok\",\"data\":{\"request\":\"vspace/one/spaceapi.state.open\"}}");
  lastCheck = millis();
}

// ##############################
// Interrupt handling
// ##############################

void interruptStateSR(){
  interruptStateCounter++;
}

void interruptStateHandler(){
  static bool last = false;

  stateOpen = digitalRead(interruptStatePin) == LOW;
  
  state.setLocalSpaceState(stateOpen ? SpaceState::SOPEN : SpaceState::SCLOSED);
  requestSpaceStateUpdate(); // Requesting remote state

  if (last != stateOpen){
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
    root["status"] = "ok";

    JsonObject& data = root.createNestedObject("data");
    data["open"] = stateOpen?true:false;

    char message[200];
    root.printTo((char*)message, root.measureLength() + 1);

    mqttClient.publish("vspace/one/state/open", message);
    last = stateOpen;
  }

  interruptStateCounter = 0;
}



// ##############################
// Arduino "main" functions
// ##############################

void setup() {
  // State switch
  pinMode(interruptStatePin, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(interruptStatePin), interruptStateSR, CHANGE);

  state.setConnectionState(ConnectionState::PRE_SERIAL);
  Serial.begin(115200);
  while (!Serial){
    ESP.wdtFeed();
    interface.loop();
  };

  state.setConnectionState(ConnectionState::PRE_WIFI);
  connectWiFi();

  mqttClient.setServer(mqtt_server, 1883);

  checkMQTTConnectionAndLoop();

  mqttClient.subscribe("vspace/one/spaceapi",1);
  mqttClient.setCallback([=](char* topic, byte* payload, unsigned int length){
    std::string s( reinterpret_cast<char const*>(payload), length);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& response = jsonBuffer.parseObject(s.c_str());

    if (!response.success()) {
      Serial.println("Error while parsing");
      return;
    }        

    const char* status = response["status"];    
    const char* request = response["data"]["request"];

    int ok = std::strcmp(status, "ok\0");
    int isRequested = std::strcmp(request, "vspace/one/spaceapi.state.open\0");

    if ((strcmp(status, "ok") == 0) 
        && (strcmp(request, "vspace/one/spaceapi.state.open") == 0)){
      const bool open = response["data"]["response"];
      //Serial.printf("Received: %s, %s, %d\n", status, request, open);
      state.setRemoteSpaceState(open ? SpaceState::SOPEN : SpaceState::SCLOSED);
    }

  });
}

void loop() {
  checkMQTTConnectionAndLoop();

  if (interruptStateCounter > 0){
    interruptStateHandler();
  }

  if ((millis() - lastCheck) > 5000){  // Check if spaceapi corresponds to local state
    requestSpaceStateUpdate();
  }

  interface.loop();
  ESP.wdtFeed();
}
