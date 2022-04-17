#pragma once

#define MAX_QUEUE 10
#define QUEUE_UPDATE_MS 1000

#include <algorithm>
#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#include "hardware.h"
#if defined (ENABLE_CMDLOG)
#include "flashLog.h"
#include "IthoSystem.h"
#endif

class IthoQueue {

  private:
    struct Queue {
      int16_t speed { -1 };
      unsigned long valid { 0 };
      void get(JsonObject, int index) const;
    };

    mutable bool firstQueueItem { true };
    struct Queue items[MAX_QUEUE];
    Ticker queueUpdater;
  public:
    uint16_t ithoSpeed;
    uint16_t ithoOldSpeed;
    uint16_t fallBackSpeed;
    void update_queue();
    bool add2queue(int speedVal, unsigned long validVal, uint8_t nonQ_cmd_clearsQ);
    void clear_queue();
    uint16_t get_itho_speed() {
      return ithoSpeed;
    };
    mutable bool ithoSpeedUpdated;
    void set_itho_fallback_speed(uint16_t speedVal) {
#if defined (ENABLE_CMDLOG)      
      char output[128]{};
      sprintf(output, "set_itho_fallback_speed - ithoSpeed:%d fallBackSpeed:%d", ithoSpeed, fallBackSpeed);
      logCmd(output);  
#endif
      fallBackSpeed = speedVal;
    };
    void get(JsonObject);
    IthoQueue();
    ~IthoQueue();

  protected:


}; //IthoQueue

extern IthoQueue ithoQueue;
