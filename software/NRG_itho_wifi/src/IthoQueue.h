#pragma once

#define MAX_QUEUE 10
#define QUEUE_UPDATE_MS 100

#include <algorithm>
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
    void get(JsonObject, int index) const;
  };

  mutable bool firstQueueItem{true};
  struct Queue items[MAX_QUEUE];
  Ticker queueUpdater;

public:
  uint16_t ithoSpeed = 0;
  uint16_t ithoOldSpeed = 0;
  uint16_t fallBackSpeed = 42;
  void update_queue();
  bool add2queue(int speedVal, unsigned long validVal, uint8_t nonQ_cmd_clearsQ);
  void clear_queue();
  uint16_t get_itho_speed()
  {
    return ithoSpeed;
  };
  mutable bool ithoSpeedUpdated = false;
  void set_itho_fallback_speed(uint16_t speedVal) { fallBackSpeed = speedVal; };
  void get(JsonObject);
  IthoQueue();
  ~IthoQueue();

protected:
}; // IthoQueue

extern IthoQueue ithoQueue;
