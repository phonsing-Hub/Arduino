#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "Adafruit_SHT31.h"

const char* ssid = "ps@2.4GHz";
const char* password = "apl-ps02";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_username = "polsing";
String pss = "APL_" + String(random(1000, 2000));
const char* mqtt_password = pss.c_str();


WiFiClient espClient;
PubSubClient client(espClient);
  
// Function to handle messages received from MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String payloadStr;
  for (int i = 0; i < length; i++) 
     payloadStr += (char)payload[i];
  Serial.println(payloadStr);
  // Parse JSON
  const size_t capacity = JSON_OBJECT_SIZE(2); // Adjust the size based on your JSON structure
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, payloadStr);

  if (doc.containsKey("LED")) {
    bool led = doc["LED"];
    if(led){
    digitalWrite(26, LOW);
    Serial.println("LED: on");
    }
    else {
    digitalWrite(26, HIGH);
    Serial.println("LED: off");
    }
    } 

  if (doc.containsKey("Relay1")) {
    bool relay1 = doc["Relay1"];
    if(relay1){
    digitalWrite(32, LOW);
    Serial.println("Relay1: on");
    }
    else{
    digitalWrite(32, HIGH);
    Serial.println("Relay1: off");
    }
  } 

    if (doc.containsKey("Relay2")) {
    bool relay2 = doc["Relay2"];
    if(relay2){
    digitalWrite(14, LOW);
    Serial.println("Relay2: on");
    }
    else{
    digitalWrite(14, HIGH);
    Serial.println("Relay2: off");
    }
  } 
}

bool enableHeater = false;
uint8_t loopCnt = 0;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
void setup() {
  Serial.begin(115200);
  while (!Serial)
  delay(10);

  pinMode(26,OUTPUT);
  pinMode(32,OUTPUT);
  pinMode(14,OUTPUT);
  digitalWrite(26,HIGH);
  digitalWrite(32, HIGH);
  digitalWrite(14, HIGH);

  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())Serial.println("ENABLED");
  else Serial.println("DISABLED");

  // Connect to Wi-Fi
  Serial.println("Connecting to WiFi ");
  WiFi.begin(ssid, password,6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected successfully");

  // Set the MQTT broker and credentials
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Connect to MQTT
  Serial.println("");
  while (!client.connected()){
    Serial.print("Connecting to MQTT: ");
    Serial.println(mqtt_server);
  if (client.connect("mqttx_5c5c2f6c", mqtt_username, mqtt_password)) {
      Serial.println("Connected to mqtt successfully");
      client.subscribe("64028780/Msg"); 
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      Serial.println("");
      delay(2000);
    }
  }
  //dht.begin();
  }

void loop() {
  // Reconnect to MQTT if connection is lost
  if (!client.connected()) {
      Serial.print("Connecting to MQTT: ");
      Serial.println(mqtt_server);
  if (client.connect("mqttx_5c5c2f6c", mqtt_username, mqtt_password)) {
      Serial.println("Connected to mqtt successfully");
      client.subscribe("64028780/Msg"); 
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      Serial.println("");
      delay(2000);
    }
  }
  float h = sht31.readHumidity();
  float t = sht31.readTemperature();

  if (!isnan(t)) {
    Serial.print("Temp *C = ");
    Serial.print(t);
    Serial.print("\t\t");
  } else Serial.println("Failed to read temperature");
  

  if (!isnan(h)) {
    Serial.print("Hum. % = ");
    Serial.println(h);
  } else Serial.println("Failed to read humidity");

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
  jsonDoc["temperature"] = t;
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  client.publish("64028780/data", jsonString.c_str());
  
  client.loop();
  delay(1000);
}
