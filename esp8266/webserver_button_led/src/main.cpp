#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>

const char* ssid = "IoTVision2_2.4GHz";// tên wifi mà bạn muốn connect
const char* password = "iotvision@2022";//pass wifi

 
int ledPin1 = 2; // GPIO2
int ledPin2 = 16;

int value1 = LOW;
int value2 = LOW;

RTC_DS3231 rtc;

WiFiServer server(80);// Port 80 
void setup() {
Serial.begin(9600);
delay(10);

while (!Serial) {
  delay(1);
}

if (! rtc.begin()){
  Serial.println("Couldn't find RTC");
  while (1);
}

if (rtc.lostPower()){
  Serial.println("RTC lost power, lets set the time!");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

//set up led
pinMode(ledPin1, OUTPUT);
pinMode(ledPin2, OUTPUT);
digitalWrite(ledPin1, HIGH);
digitalWrite(ledPin2, HIGH);
 
// Kết nỗi với wifi
Serial.println();
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
 
// Bắt đầu sever
server.begin();
Serial.println("Server started");
 
// In địa chỉ IP 
Serial.print("Use this URL to connect: ");
Serial.print("http://");
Serial.print(WiFi.localIP());
Serial.println("/");
 

}
 
void loop() {
DateTime now = rtc.now();

// Kiểm tra xem đã connect chưa
WiFiClient client = server.available();
if (!client) {
return;
}
 
// Đọc data
Serial.println("new client");
while(!client.available()){
delay(1);
}
 
String request = client.readStringUntil('\r');
Serial.println(request);
client.flush();

if (request.indexOf("/LED1=ON") != -1) {
digitalWrite(ledPin1, LOW);
value1 = HIGH;
}
if (request.indexOf("/LED1=OFF") != -1) {
digitalWrite(ledPin1, HIGH);
value1 = LOW;
}
if (request.indexOf("/LED2=ON") != -1){
  digitalWrite(ledPin2, LOW);
  value2 = HIGH;
}
if (request.indexOf("/LED2=OFF") != -1){
  digitalWrite(ledPin2, HIGH);
  value2 = LOW;
}
//digitalWrite(ledPin, value);
 
 
// Tạo giao diện cho html!!! giống con elthernet shield á!!! bạn có thể thiết kế 1 giao diện html khác, cho đẹp
client.println("HTTP/1.1 200 OK");
client.println("Content-Type: text/html");
client.println(""); // do not forget this one
client.println("<!DOCTYPE HTML>");
client.println("<html>");
 
client.print("Led pin1 is now: ");
 
if(value1 == HIGH) {
client.print("On");
} else {
client.print("Off");
}
client.println("<br><br>");
client.print("Led pin2 is now: ");
if (value2 == HIGH){
  client.print("On");
}
else{
  client.print("Off");
}

//display time now on webserver continuosly
client.println("<!DOCTYPE HTML>");
client.println("<html>");
client.println("<br><br>");
client.print("Time now is: ");
client.print(now.hour(), DEC);
client.print(':');
client.print(now.minute(), DEC);
client.print(':');
client.print(now.second(), DEC);
client.println();
client.println("</html>");

client.println("<br><br>");
client.println("Click <a href=\"/LED1=ON\">here</a> turn the LED on pin 1 ON<br>");
client.println("Click <a href=\"/LED1=OFF\">here</a> turn the LED off pin 1 OFF<br>");
client.println("Click <a href=\"/LED2=ON\">here</a> turn the LED on pin 2 ON<br>");
client.println("Click <a href=\"/LED2=OFF\">here</a> turn the LED off pin 2 OFF<br>");
client.println("</html>");
 
delay(1);
Serial.println("Client disonnected");
Serial.println("");
}