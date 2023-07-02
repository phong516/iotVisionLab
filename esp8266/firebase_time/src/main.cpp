#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <ESP8266WiFiMulti.h>
#include <RTClib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <LiquidCrystal_I2C.h>
// #include "ds3231.h"
// #include "sht31.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

//multiwifi and firebase for anonymous login
ESP8266WiFiMulti wifiMulti;
FirebaseData fbdo;

//firebase config
FirebaseConfig config;

//firebase realtime database
#define DATABASE_URL "https://nodemcufirebase-daa20-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define API_KEY "AIzaSyC9A7oxs48kjrTC8QnhqvEcRpmxi1_TdgY"
//firebase auth
FirebaseAuth auth;

RTC_DS3231 rtc;
Adafruit_SHT31 sht31;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define ledTimer  2
#define ledTemp  16
#define ledHum   0
#define on        0
#define off       1

typedef struct{
  int hour;
  int minute;
  int second;
}timerFB_t;

timerFB_t timerFrom;
timerFB_t timerTo;
timerFB_t timePrev;

typedef struct{
  float temp = 30.0;
  float hum = 60.0;
}sht31Data_t;

sht31Data_t sht31Data;

typedef struct{
  String ledTimerState = "OFF";
  String ledTempState = "OFF";
  String ledHumState = "OFF";
}led_t;

typedef struct{
  float up;
  float down;
}limit_t;

limit_t tempLimit;
limit_t humLimit;

unsigned long sendTimePrev = 0;
unsigned long sendSHT31Prev = 0;
unsigned long sendLCDPrev = 0;

static void initLCD();
static void initLed();
static void initWifi();
static void initSHT31();
static void initRTC();
static void initFirebase();

static void initTime(String topic);
static void initTimer(String topic, int hour, int minute, int second);
static void sendTimeToFirebase(String topic, timerFB_t *timePrev);
static void checkTimer(timerFB_t * from, timerFB_t * to);
static void readTimerFromFirebase(String topic, timerFB_t * timerUP, timerFB_t * timerDown);

static void initSHT31Limit(float tempUp, float tempDown, float humUp, float humDown);
static void readSHT31Data(sht31Data_t *sht31Data);
static void sendSHT31ToFirebase(String topic);
static void readSHT31LimitFromFirebase(String topic);
static void checkSHT31(limit_t *tempLimit, limit_t *humLimit);

static void sendLedStateToFirebase(String topic);

static void sendLCD(String str1, int col, int row);

void setup() {
  Serial.begin(115200);
  initLCD();
  initLed();
  initSHT31();
  initRTC();
  initWifi();
  initFirebase();
  initTime("time");
  initTimer("time/timer/from", rtc.now().hour(), rtc.now().minute() + 2, 0);
  initTimer("time/timer/to", rtc.now().hour(), rtc.now().minute() + 3, 0);
  initSHT31Limit(30, 20, 80, 60);
}

void loop() {

  /////////////////////////////////////////////////////////
  /*  send temperate and humidity data to firebase*/
  if (millis() - sendSHT31Prev > 1999){
    readSHT31Data(&sht31Data);
    sendSHT31Prev = millis();
    sendSHT31ToFirebase("sht31");
    readSHT31LimitFromFirebase("sht31/limit");
    checkSHT31(&tempLimit, &humLimit);
    sendLedStateToFirebase("led");
  }
  /////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////
  /*  send time from ds3231 to firebase*/
  if (millis() - sendTimePrev> 2999){
    sendTimePrev = millis();
    sendTimeToFirebase("time", &timePrev);
    readTimerFromFirebase("time/timer", &timerFrom, &timerTo);
    checkTimer(&timerFrom, &timerTo);
    sendLedStateToFirebase("led");
  }
  /////////////////////////////////////////////////

  /////////////////////////////////////////////////////////
  /* print lcd*/
  if (millis() - sendLCDPrev > 3999){
    sendLCDPrev = millis();
    DateTime now = rtc.now();
    sendLCD("Time: " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()), 0, 0);
    sendLCD(String(sht31Data.temp, 2) + "oC" + " " + String(sht31Data.hum, 2) + "%", 0, 1);
  }
}

static void initLCD(){
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

static void initLed(){

  pinMode(ledTimer, OUTPUT);
  pinMode(ledTemp, OUTPUT);
  pinMode(ledHum, OUTPUT);

  digitalWrite(ledTimer, off);
  digitalWrite(ledTemp, off);
  digitalWrite(ledHum, LOW);
}

static void initWifi(){
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("IoTVision2_2.4GHz", "iotvision@2022");
  wifiMulti.addAP("homebangder", "homelander");
  Serial.println("Connecting Wifi...");
  while (wifiMulti.run(WIFI_CONNECT_TIMEOUT_MS) != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nConnected to Wifi: ");
  Serial.println(WiFi.SSID());
}

static void initRTC(){
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return;
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    //plus 30 seconds to the time you set to avoid the time lag in uploading
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)).unixtime() + 50);  }
  else{
    Serial.println("RTC has set the time!");
  }
}

static void initFirebase(){
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

static void initTime(String topic){
  FirebaseJson json;
  DateTime now = rtc.now();
  json.set("hour", now.hour());
  json.set("minute", now.minute());
  json.set("second", now.second());
  Firebase.RTDB.updateNode(&fbdo, topic, &json);

  timePrev = {
    .hour = now.hour(),
    .minute = now.minute(),
    .second = now.second()
  };
}

static void initTimer(String topic, int hour, int minute, int second){
  if (hour > 23) hour -= 24;
  if (minute > 59) minute -= 60;
  if (second > 59) second -= 60;

  FirebaseJson json;
  json.set("hour", hour);
  json.set("minute", minute);
  json.set("second", second);
  Firebase.RTDB.updateNode(&fbdo, topic, &json);
}

static void sendTimeToFirebase(String topic, timerFB_t *timePrev){
  FirebaseJson json;
  DateTime now = rtc.now();
  if (timePrev->hour != now.hour() && now.hour() < 24){
    json.set("hour", now.hour());
    timePrev->hour = now.hour();
  }
  if (timePrev->minute != now.minute() && now.minute() < 60){
    json.set("minute", now.minute());
    timePrev->minute = now.minute();
  }
  if (timePrev->second != now.second() && now.second() < 60){
    json.set("second", now.second());
    timePrev->second = now.second();
  }
  Firebase.RTDB.updateNode(&fbdo, topic, &json);
}

static void checkTimer(timerFB_t * from, timerFB_t * to){
  DateTime now = rtc.now();
  uint32_t toSum = to->hour * 3600 + to->minute * 60 + to->second;
  uint32_t fromSum = from->hour * 3600 + from->minute * 60 + from->second;
  uint32_t nowSum = now.hour() * 3600 + now.minute() * 60 + now.second();
  if (fromSum <= nowSum && nowSum <= toSum){
    digitalWrite(ledTimer, on);
  }
  else{
    digitalWrite(ledTimer, off);
  }
}

static void readTimerFromFirebase(String topic, timerFB_t *from, timerFB_t *to){
  if (Firebase.RTDB.getJSON(&fbdo, topic)){
    FirebaseJson *json = fbdo.jsonObjectPtr();
    FirebaseJsonData jsonData;

    if (json->get(jsonData, "from/hour"))
      from->hour = jsonData.intValue;
    if (json->get(jsonData, "from/minute"))
      from->minute = jsonData.intValue;
    if (json->get(jsonData, "from/second"))
      from->second = jsonData.intValue;

    if (json->get(jsonData, "to/hour"))
      to->hour = jsonData.intValue;
    if (json->get(jsonData, "to/minute"))
      to->minute = jsonData.intValue;
    if (json->get(jsonData, "to/second"))
      to->second = jsonData.intValue;
  }
}

static void initSHT31(){
  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
  }
}

static void initSHT31Limit(float tempUp, float tempDown, float humUp, float humDown){
  FirebaseJson json;
  json.set("temp/up", tempUp);
  json.set("temp/down", tempDown);
  json.set("hum/up", humUp);
  json.set("hum/down", humDown);
  Firebase.RTDB.updateNode(&fbdo, "sht31/limit", &json);
}

static void readSHT31Data(sht31Data_t *sht31Data){
  sht31Data->temp = sht31.readTemperature();
  sht31Data->hum = sht31.readHumidity();
}

static void sendSHT31ToFirebase(String topic){
  FirebaseJson json;
  json.set("temp", sht31Data.temp);
  json.set("hum", sht31Data.hum);
  Firebase.RTDB.updateNode(&fbdo, topic, &json);
}

static void readSHT31LimitFromFirebase(String topic){
  if (Firebase.RTDB.getJSON(&fbdo, topic)){
    FirebaseJson *json = fbdo.jsonObjectPtr();
    FirebaseJsonData jsonData;
    if (json->get(jsonData, "temp/up"))
      tempLimit.up = jsonData.floatValue;
    if (json->get(jsonData, "temp/down"))
      tempLimit.down = jsonData.floatValue;
    if (json->get(jsonData, "hum/up"))
      humLimit.up = jsonData.floatValue;
    if (json->get(jsonData, "hum/down"))
      humLimit.down = jsonData.floatValue;
  }
}

static void checkSHT31(limit_t *tempLim, limit_t *humLim){
  if (tempLim->down < sht31Data.temp && sht31Data.temp > tempLim->down)
    digitalWrite(ledTemp, on);
  if (tempLim->down > sht31Data.temp && sht31Data.temp < tempLim->up)
    digitalWrite(ledTemp, off);

  if (humLim->up < sht31Data.hum && sht31Data.hum > humLim->down)
    digitalWrite(ledHum, HIGH);
  if (humLim->down > sht31Data.hum && sht31Data.hum < humLim->up)
    digitalWrite(ledHum, LOW);
}

static void sendLedStateToFirebase(String topic){
  led_t ledState = {
    .ledTimerState = digitalRead(ledTimer) == on ? "ON" : "OFF",
    .ledTempState = digitalRead(ledTemp) == on ? "ON" : "OFF",
    .ledHumState = digitalRead(ledHum) == HIGH ? "ON" : "OFF"};
  FirebaseJson json;
  json.set("ledTimer", ledState.ledTimerState);
  json.set("ledTemp", ledState.ledTempState);
  json.set("ledHum", ledState.ledHumState);
  Firebase.RTDB.updateNode(&fbdo, topic, &json);
}

static void sendLCD(String str, int col, int row){
  lcd.setCursor(col, row);
  lcd.print(str);
}