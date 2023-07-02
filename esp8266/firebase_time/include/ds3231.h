#ifndef __DS3231_H__
#define __DS3231_H__

#include <Arduino.h>
#include <RTClib.h>

typedef struct{
  int hour;
  int minute;
  int second;
}timerFB_t;

static void initRTC();
static void initTimer(String topic, int hour, int minute, int second);
static void sendTimeToFirebase(String topic);
static void checkTimer(timerFB_t * from, timerFB_t * to);
static void readTimerFromFirebase(String topic, timerFB_t * timerUP, timerFB_t * timerDown);

#endif // __DS3231_H__