#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "SparkFunCCS811.h" 
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include "variab.h"
#define CCS811_ADDR 0x5B //Default I2C Address

//insert the ssid and password for your Wi-Fi network
// const char* ssid = "";
// const char* password = "";

//instead, use ssid and password from file
const char* ssid = ssid_f;
const char* password = password_f;


// MICS-6814 sensor Board. Assigning ESp32 pins
int pinNO2 = 34;
int pinNH3 = 39;
int pinCO = 36;

//CCS811 uses I2C communication interface, therefore we do not need to assign other pins.

////////Assign measure units for each parameter

const char* unitCO2 = "ppm";
const char* unitTVOC = "ppm";
const char* unitNO2 = "V";
const char* unitNH3 = "V";
const char* unitCO = "V";


////////Assign measurement type (parameter)
const char* nameCO2 = "Carbon Dioxide";
const char* nameTVOC = "Total Volatile Organic Compounds";
const char* nameNO2 = "Nitrogen Dioxide";
const char* nameNH3 = "Ammonia";
const char* nameCO = "Carbon Monoxide";


////////Assign mqtt server IP from file
const char* mqtt_server = mqtt_server_f;
////////Otherwise, assign mqtt server IP here
// const char* mqtt_server = "";

// Declare the objects for NTP server communication
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Declare the objects for MQTT and Wi-Fi client
WiFiClient espClient;
PubSubClient client(espClient);

//Create the topics for the sensors
const char* gen_topic =  "hubcap/sensors/";
const char* topicNO2 ="hubcap/sensors/NO2";
const char* topicNH3 ="hubcap/sensors/NH3";
const char* topicCO ="hubcap/sensors/CO";
const char* topicTVOC ="hubcap/sensors/CCS811_TVOC";
const char* topicCO2 ="hubcap/sensors/CCS811_CO2";

//Declare CCS811 object for CCs811 sensor communication through I2C
CCS811 mySensor(CCS811_ADDR);

//// Function to format the data according to HUBCAP JSON format
//tip: measurement type (parameter)
//valoare: value of the parameter
//unitate: measurement unit of the parameter
//output: char buffer to store the JSON. It is modified by json_format function.
void json_format(const char* tip, float valoare, const char* unitate, char* output)
{
// JSON Document Length must be at least 768 bytes
StaticJsonDocument<1000> doc;
//Call getFormattedDate to take the formattedDate as, for example, 2022-06-08T11:43:42Z.
String formattedDate = timeClient.getFormattedDate();
//Define and assign the JSON fields
doc["deviceUUID"] = "ESP32-1-1";
doc["timestamp"] = formattedDate;
 // Create a nested array to store the JSON objects for parameter type, parameter value and measure unit.
JsonArray infos = doc.createNestedArray("infos"); 
JsonObject infos_0 = infos.createNestedObject();
infos_0["type"] = tip;
infos_0["value"] = valoare;
infos_0["unit"] = unitate;
//serialize the JSON doc to prepare it for MQTT publishing
serializeJson(doc, output, 256);
//update NTP time client
while(!timeClient.update()) {
    timeClient.forceUpdate();}
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPClient")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("RSSI: ");
  Serial.println(WiFi.RSSI());
}

void setup() {
  Serial.begin(115200);
  // timeClient.begin(); causes continuous rebooting
  ///set +3 timezone
  timeClient.setTimeOffset(10800); 
  //Start I2C
   Wire.begin();
   setup_wifi();
   client.setServer(mqtt_server, 1883);
   pinMode(pinCO, INPUT);
   pinMode(pinNH3, INPUT);
   pinMode(pinNO2, INPUT);

   while (mySensor.begin() == false)
  {
    Serial.print("CCS811 error. Please check wiring. Freezing...");
    
  }
  Serial.print("CCS811 connected");
  Serial.println();
  Serial.print("CO2, tVOC, CO, NH3, NO2, millis");
  Serial.println();
}


void fun_publish(float Sensorvalue, const char* topicSensor, const char* SensorUnit, const char* nameSensor){
  ///function to publish MQTT JSON-formatted payloads
    char output[256];
    json_format(nameSensor, Sensorvalue, SensorUnit, output);
    Serial.println("payload:");
    Serial.println(output);
    Serial.println("################################3");
    client.publish(topicSensor, output);
    
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(200);
const float max_volt = 1.1;
const float quan_M = 4095.0;
  float valueCO = max_volt*analogRead(pinCO)/quan_M;
  float valueNH3 = max_volt*analogRead(pinNH3)/quan_M;
  float valueNO2 = max_volt*analogRead(pinNO2)/quan_M;
  //for testing only
  // Serial.println(valueCO);
  // Serial.println(valueNH3);
  // Serial.println(valueNO2);
  // delay(1000);
  if (mySensor.dataAvailable())
  {
    
    mySensor.readAlgorithmResults();

    //Returns calculated CO2 reading
    int valueCO2 = mySensor.getCO2();
    int valueTVOC = mySensor.getTVOC();
    Serial.print(valueCO2);
    Serial.print(",");
    //Returns calculated TVOC reading
    Serial.print(valueTVOC);
    Serial.print(",");
    Serial.print(valueCO);
    Serial.print(",");
    Serial.print(valueNH3);
    Serial.print(",");
    Serial.print(valueNO2);
    Serial.print(",");
    //Display the time since program start
    Serial.println();
  delay(10); //Don't spam the I2C bus

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
fun_publish(valueCO2, topicCO2, unitCO2, nameCO2);
fun_publish(valueTVOC, topicTVOC, unitTVOC, nameTVOC);
fun_publish(valueCO, topicCO, unitCO, nameCO);
fun_publish(valueNO2, topicNO2, unitNO2, nameNO2);
fun_publish(valueNH3, topicNH3, unitNH3, nameNH3);
delay(1000);
  }
}