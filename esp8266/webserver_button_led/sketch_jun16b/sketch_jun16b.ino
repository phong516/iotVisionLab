#include <Wire.h>
#include "Adafruit_SHT31.h"
Adafruit_SHT31 sht31;
void setup() {
  //set dia chi
  Serial.begin(9600);
  if(!sht31.begin(0x44)){   //địa chỉ mặc định là 0x44 ,nếu muốn vẫn có thể set lên 0x45
    //nếu không tìm thấy địa chỉ của sht31 thì in ra
    Serial.println("Khong tim thay sht");
    while(1) delay(1000);
    }
}

void loop() {
  //khai báo đọc nhiệt độ
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  Serial.print(t);
}