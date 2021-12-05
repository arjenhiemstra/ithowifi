#pragma once

#define MAX_NUMBER_OF_REMOTES 10
#define REMOTE_CONFIG_VERSION "002"

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#include "IthoPacket.h"

class IthoRemote {
  private:
    struct Remote {
      uint8_t ID[3] { 0, 0, 0 };
      char name[32];
      StaticJsonDocument<128> capabilities;
      void set(JsonObjectConst);
      void get(JsonObject, int index) const;
    };
    Remote remotes[MAX_NUMBER_OF_REMOTES];
    int remoteCount { 0 };
    mutable bool llMode = false;
    //Ticker timer;
    volatile uint8_t llModeTime { 0 };
    //functions
  public:
    IthoRemote();
    ~IthoRemote();
    int getRemoteCount();
    //mutable volatile bool llModeTimerUpdated { false };    
    bool toggleLearnLeaveMode();
    bool remoteLearnLeaveStatus() {
      return llMode;
    };
    void updatellModeTimer();
    uint8_t getllModeTime() {
      return llModeTime;
    };
    void setllModeTime(const int timeVal) {
      llModeTime = timeVal;
    };
    int registerNewRemote(const int* id);
    int removeRemote(const int* id);
    int removeRemote(const uint8_t index);
    void addCapabilities(const uint8_t remoteIndex, const char* name, int32_t value);
    void updateRemoteName(const uint8_t index, const char* remoteName);
    int remoteIndex(const int32_t id);
    int remoteIndex(const int* id);
    const int * getRemoteIDbyIndex(const int index);
    const char * getRemoteNamebyIndex(const int index);
    const char * lastRemoteName;
    bool checkID(const int* id);
    bool configLoaded;
    char config_struct_version[4];
    bool set(JsonObjectConst);
    void get(JsonObject) const;
    void getCapabilities(JsonObject obj) const;
  protected:


}; //IthoRemote
