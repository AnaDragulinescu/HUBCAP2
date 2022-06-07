#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "SparkFunCCS811.h" 
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>//Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#define CCS811_ADDR 0x5B //Default I2C Address


int pinNO2 = 34;
int pinNH3 = 39;
int pinCO = 36;

const char* unitCO2 = "ppm";
const char* unitTVOC = "ppm";
const char* unitNO2 = "V";
const char* unitNH3 = "V";
const char* unitCO = "V";


const char* nameCO2 = "Carbon Dioxide";
const char* nameTVOC = "Total Volatile Organic Compounds";
const char* nameNO2 = "Nitrogen Dioxide";
const char* nameNH3 = "Ammonia";
const char* nameCO = "Carbon Monoxide";

// const char* mqtt_server = "195.201.227.170";
const char* mqtt_server = "141.85.161.86";
// const char* mqtt_server = "127.0.0.1";
// const char* ssid = "comm.pub.ro1";
// const char* password = "c4t3dr4c0mm";

const char* ssid = "SmartAgro";
const char* password = "SA123456";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


WiFiClient espClient;
PubSubClient client(espClient);

int value = 0;
const char* gen_topic =  "hubcap/sensors/";
const char* topicNO2 ="hubcap/sensors/NO2";
const char* topicNH3 ="hubcap/sensors/NH3";
const char* topicCO ="hubcap/sensors/CO";
const char* topicTVOC ="hubcap/sensors/CCS811_TVOC";
const char* topicCO2 ="hubcap/sensors/CCS811_CO2";





CCS811 mySensor(CCS811_ADDR);


void json_format(const char* tip, float valoare, const char* unitate, char* output)
{

StaticJsonDocument<1000> doc;

String formattedDate = timeClient.getFormattedDate();

doc["deviceUUID"] = "ESP32-1-1";
doc["timestamp"] = formattedDate;
 
JsonArray infos = doc.createNestedArray("infos");
 
JsonObject infos_0 = infos.createNestedObject();
infos_0["type"] = tip;
infos_0["value"] = valoare;
infos_0["unit"] = unitate;

// char output[256];
serializeJson(doc, output, 256);


// char buffer[512];

while(!timeClient.update()) {
    timeClient.forceUpdate();
  }


// JsonObject infos = doc.createNestedObject("infos");
// infos["type"]=tip;
// infos["value"]=valoare;
// infos["unit"]=unitate;

  // serializeJsonPretty(doc, buffer); 
  
  // Serial.println(buf);
  // return output;
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
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // timeClient.begin();
  timeClient.setTimeOffset(10800);
   Wire.begin();
   delay(1000);
   setup_wifi();
   delay(1000);
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
  
    // dtostrf(Sensorvalue, 1, 2, SensorString);
    // sprintf(buf, "%s value: ", nameSensor);
    // Serial.print(buf);
    // Serial.println(SensorString);

    char output[256];
    json_format(nameSensor, Sensorvalue, SensorUnit, output);
    // const char* payload = json_format(nameSensor, Sensorvalue, SensorUnit);

    
    
    Serial.println("payload:");
    Serial.println(output);
    Serial.println("################################3");
    // Serial.print(&payload);
    client.publish(topicSensor, output);
    
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(200);
  int valueCO = 5*analogRead(pinCO)/1023;
  int valueNH3 = 5*analogRead(pinNH3)/1023;
  int valueNO2 = 5*analogRead(pinNO2)/1023;
  
  if (mySensor.dataAvailable())
  {
    //If so, have the sensor read and calculate the results.
    //Get them later
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
    Serial.print(millis());
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
// fun_publish(valueCO2, topicCO2);
// fun_publish(valueCO2, topicCO2);

  }
}