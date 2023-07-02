/*
  Rui Santos
  Complete project details at our blog.
    - ESP32: https://RandomNerdTutorials.com/esp32-firebase-realtime-database/
    - ESP8266: https://RandomNerdTutorials.com/esp8266-nodemcu-firebase-realtime-database/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based in the RTDB Basic Example by Firebase-ESP-Client library by mobizt
  https://github.com/mobizt/Firebase-ESP-Client/blob/main/examples/RTDB/Basic/Basic.ino
*/

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFiMulti.h>
#endif
#include <Firebase_ESP_Client.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>

ESP8266WiFiMulti wifiMulti;
Adafruit_SHT31 sht; // I2C

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyC9A7oxs48kjrTC8QnhqvEcRpmxi1_TdgY"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://nodemcufirebase-daa20-default-rtdb.asia-southeast1.firebasedatabase.app/" 

#define tempLed 2
#define humLed 16

#define on 0
#define off 1

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

typedef struct sht31Data {
  float temperature;
  float humidity;
}sht31Data_t;

typedef struct limit{
  float tempMax;
  float tempMin;
  float humMax;
  float humMin;
}limit_t;

void initWifi();
void initSHT31();
sht31Data_t readSHT31();
void sendDataToFirebase(sht31Data_t data);
limit_t readLimitFromFirebase();
void checkLimit(sht31Data_t data, limit_t limit);
void initLimit(float tempMax, float tempMin, float humMax, float humMin);

void setup(){
  pinMode(tempLed, OUTPUT);
  pinMode(humLed, OUTPUT);
  digitalWrite(tempLed, off);
  digitalWrite(humLed, off);
  Serial.begin(115200);
  initWifi();
  initSHT31();
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  initLimit(30.0, 20.0, 80.0, 60.0);
}

void loop(){
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000)){
    sendDataPrevMillis = millis();
    sht31Data_t data = readSHT31();
    sendDataToFirebase(data);
    limit_t limit = readLimitFromFirebase();
    checkLimit(data, limit);
  }
}

void initWifi(){
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("IoTVision2_2.4GHz", "iotvision@2022");
  wifiMulti.addAP("homebangder", "homelander");
  Serial.print("Connecting to Wi-Fi");
  while (wifiMulti.run(WIFI_CONNECT_TIMEOUT_MS) != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to the Wifi network: ");
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.localIP());
}

void initSHT31(){
  if (! sht.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
}

sht31Data_t readSHT31(){
  sht31Data_t sht31Data;
  sht31Data.temperature = sht.readTemperature();
  sht31Data.humidity = sht.readHumidity();
  return sht31Data;
}

void sendDataToFirebase(sht31Data_t data){
  Firebase.RTDB.setFloat(&fbdo, "data/temperature", data.temperature);
  Firebase.RTDB.setFloat(&fbdo, "data/humidity", data.humidity);
}

limit_t readLimitFromFirebase(){
  limit_t limit;
  if (Firebase.RTDB.getFloat(&fbdo, "limit/temperature/up")){
    if (fbdo.dataType() == "float" || fbdo.dataType() == "int"){
      limit.tempMax = fbdo.floatData();

    }
  }
  if (Firebase.RTDB.getFloat(&fbdo, "limit/temperature/down")){
    if (fbdo.dataType() == "float" || fbdo.dataType() == "int"){
      limit.tempMin = fbdo.floatData();

    }
  }
  if (Firebase.RTDB.getFloat(&fbdo, "limit/humidity/up")){
    if (fbdo.dataType() == "float" || fbdo.dataType() == "int"){
      limit.humMax = fbdo.floatData();

    }
  }
  if (Firebase.RTDB.getFloat(&fbdo, "limit/humidity/down")){
    if (fbdo.dataType() == "float" || fbdo.dataType() == "int"){
      limit.humMin = fbdo.floatData();
    }
  }
  return limit;
}

void checkLimit(sht31Data_t data, limit_t limit){
  if (data.temperature > limit.tempMax){
    digitalWrite(tempLed, on);
  }
  if (data.temperature < limit.tempMin){
    digitalWrite(tempLed, off);
  }
  if (data.humidity > limit.humMax){
    digitalWrite(humLed, on);
  }
  if (data.humidity < limit.humMin){
    digitalWrite(humLed, off);
  }
}

void initLimit(float tempMax, float tempMin, float humMax, float humMin){
  Firebase.RTDB.setFloat(&fbdo, "limit/temperature/down", tempMin);
  Firebase.RTDB.setFloat(&fbdo, "limit/temperature/up", tempMax);
  Firebase.RTDB.setFloat(&fbdo, "limit/humidity/down", humMin);
  Firebase.RTDB.setFloat(&fbdo, "limit/humidity/up", humMax);
}