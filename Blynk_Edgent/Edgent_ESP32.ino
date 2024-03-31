#define BLYNK_TEMPLATE_ID "TMPL662ZlFUAX"
#define BLYNK_TEMPLATE_NAME "CPE345x64028780"
#define BLYNK_FIRMWARE_VERSION        "0.2.1"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include "BlynkEdgent.h"
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <PubSubClient.h>
#include <ArduinoJson.h> 

#define BLYNK_VIRTUAL_PIN_TEMPERATURE V1
#define BLYNK_VIRTUAL_PIN_HUMIDITY V2

#define LED_PIN 27
#define RELAY_PIN 32

bool relayState = HIGH; 
bool enableHeater = false;
uint8_t loopCnt = 0;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_username = "polsing";
const char* mqtt_password = "150402";


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(100);
  BlynkEdgent.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  while (!Serial)
    delay(10);

  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");

  // Connect to MQTT
  Serial.println("");
  while (!client.connected()){
    Serial.print("Connecting to MQTT: ");
    Serial.println(mqtt_server);
  if (client.connect("mqttx_5c5c2f6c", mqtt_username, mqtt_password)) {
      Serial.println("Connected to mqtt successfully");
      client.subscribe("onLED");  // Subscribe to topic "msg/"
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      Serial.println("");
      delay(2000);
    }
  }
}

void loop() {
  BlynkEdgent.run();

  if (Blynk.connected())
    digitalWrite(LED_PIN, LOW);
  else
    digitalWrite(LED_PIN, HIGH);

      // Reconnect to MQTT if connection is lost
  if (!client.connected()) {
      Serial.print("Connecting to MQTT: ");
      Serial.println(mqtt_server);
  if (client.connect("mqttx_5c5c2f6c", mqtt_username, mqtt_password)) {
      Serial.println("Connected to mqtt successfully");
      client.subscribe("onLED");  // Subscribe to topic "msg/"
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      Serial.println("");
      delay(2000);
    }
  }

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(t)) {
    Serial.print("Temp *C = ");
    Serial.print(t);
    Serial.print("\t\t");
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_TEMPERATURE, t); // ส่งค่าอุณหภูมิขึ้น Blynk
    //publishMQTT("64028780/Temp", String(t)); // Publish temperature to MQTT broker
  } else {
    Serial.println("Failed to read temperature");
  }

  if (!isnan(h)) {
    Serial.print("Hum. % = ");
    Serial.println(h);
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_HUMIDITY, h); // ส่งค่าความชื้นขึ้น Blynk
    //publishMQTT("64028780/Hum", String(h)); // Publish humidity to MQTT broker
  } else {
    Serial.println("Failed to read humidity");
  }

  delay(1000);

  // Toggle heater enabled state every 30 seconds
  // An ~3.0 degC temperature increase can be noted when heater is enabled
  if (loopCnt >= 30) {
    enableHeater = !enableHeater;
    sht31.heater(enableHeater);
    Serial.print("Heater Enabled State: ");
    if (sht31.isHeaterEnabled())
      Serial.println("ENABLED");
    else
      Serial.println("DISABLED");

    loopCnt = 0;
  }
  loopCnt++;

 
  DynamicJsonDocument jsonDoc(200);
  jsonDoc["humidity"] = h;
  jsonDoc["temperature_C"] = t;
  //jsonDoc["temperature_F"] = f;
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  client.publish("formData", jsonString.c_str());
  
  client.loop();
  delay(1000);
}


BLYNK_WRITE(V0) {
  int relayControl = param.asInt();
  if (relayControl) {
    controlRelay(LOW); 
    Serial.println("Relay: on");
  }
  else {
    controlRelay(HIGH);
    Serial.println("Relay: off");
  }
}

void controlRelay(bool state) {
  digitalWrite(RELAY_PIN, state);
  relayState = state;
}
