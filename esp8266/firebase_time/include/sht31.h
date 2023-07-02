#ifndef __SHT31_H__
#define __SHT31_H__

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>

typedef struct{
  float temp;
  float hum;
}sht31Data_t;

static void initSHT31Limit(float tempUp, float tempDown, float humUp, float humDown);
static void readSHT31Data();
static void sendSHT31ToFirebase(String topic);
static void readSHT31LimitFromFirebase(String topic);
static void checkSHT31(limit_t *tempLimit, limit_t *humLimit);



#endif // __SHT31_H__