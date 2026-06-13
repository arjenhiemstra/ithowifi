#pragma once

#define MAX_QUEUE 10
#define QUEUE_UPDATE_MS 100

//#include <algorithm>
#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Ticker.h>

class IthoQueue
{

private:
  struct Queue
  {
    int16_t speed{-1};
    unsigned long valid{0};
    void get(JsonObject obj, int index) const;
  };

  mutable bool firstQueueItem{true};
  struct Queue items[MAX_QUEUE];
  //Ticker queueUpdater;

public:
  uint16_t ithoSpeed = 0;
  uint16_t ithoOldSpeed = 0;
  uint16_t fallBackSpeed = 42;
  void updateQueue();
  bool addToQueue(int speedVal, unsigned long validVal, uint8_t nonQ_cmd_clearsQ);
  void clearQueue();
  uint16_t get_itho_speed()
  {
    return ithoSpeed;
  };
  // Head-of-queue timer (the next entry to expire). Used by /api/v2/speed
  // to surface the add-on-tracked timer remaining for PWM2I2C timer
  // commands (which the Itho unit itself doesn't know about). Returns
  // 0 ms / -1 speed when no active timer is queued.
  void getHeadTimer(unsigned long &remainingMs, int16_t &speed) const
  {
    remainingMs = items[0].valid;
    speed = items[0].speed;
  }
  mutable bool ithoSpeedUpdated = false;
  void set_itho_fallback_speed(uint16_t speedVal) { fallBackSpeed = speedVal; };
  void get(JsonArray arr);
  IthoQueue();
  ~IthoQueue();

protected:
}; // IthoQueue

extern IthoQueue ithoQueue;
