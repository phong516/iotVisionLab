/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp8266-web-server-gauges/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <RTClib.h>
#include <SPI.h>
#include <ESP8266WiFiMulti.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>

ESP8266WiFiMulti wifiMulti;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 1000;

// Create a sensor object
RTC_DS3231 rtc;

Adafruit_SHT31 sht; // I2C

void initSHT(){
  if (! sht.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
}

void initRTC(){
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()){
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

String getTime(String command){
  DateTime now = rtc.now();
  if(command == "hour"){
    String output = (now.hour()<10)?"0":""; //if hour is less than 10, add a 0 before the number
    output += String(now.hour()); //add the hour
    return output;
  }
  else if(command == "minute"){
    String output = (now.minute()<10)?"0":""; //if minute is less than 10, add a 0 before the number
    output += String(now.minute()); //add the minute
    return output;
  }
  else if(command == "second"){
    String output = (now.second()<10)?"0":""; //if second is less than 10, add a 0 before the number
    output += String(now.second()); //add the second
    return output;
  }
  else{
    return "error"; //if input is invalid, return error
  }
}

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  readings["temperature"] = String(sht.readTemperature());
  readings["humidity"] =  String(sht.readHumidity());
  readings["time"] = getTime("hour") + ":" + getTime("minute") + ":" + getTime("second");
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("IoTVision2_2.4GHz", "iotvision@2022");
  wifiMulti.addAP("homebangder", "homelander");
  wifiMulti.addAP("IoTVision2_5GHz", "iotvision@2022");
  wifiMulti.addAP("TUZAP", "tu123456");

  Serial.print("Connecting to WiFi ..");
  while (wifiMulti.run(WIFI_CONNECT_TIMEOUT_MS) != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("\nConnected to the WiFi network: ");
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.localIP());

}

void setup() {
  Serial.begin(9600);
  initRTC();
  initSHT();
  initWiFi();
  initFS();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");
  
  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 1000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 1 second
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }
}