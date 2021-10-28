#pragma once

#define MAX_NUMBER_OF_REMOTES 10
#define REMOTE_CONFIG_VERSION "002"

#include <stdio.h>
#include <string.h>
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
    void setllModeTime(int timeVal) {
      llModeTime = timeVal;
    };
    int registerNewRemote(int* id);
    int removeRemote(int* id);
    int removeRemote(uint8_t index);
    void addCapabilities(uint8_t remoteIndex, const char* name, int32_t value);
    void updateRemoteName(uint8_t index, const char* remoteName);
    int remoteIndex(int32_t id);
    int remoteIndex(int* id);
    int * getRemoteIDbyIndex(int index);
    char * getRemoteNamebyIndex(int index);
    char * lastRemoteName;
    bool checkID(int* id);
    bool configLoaded;
    char config_struct_version[4];
    bool set(JsonObjectConst);
    void get(JsonObject) const;
    void getCapabilities(JsonObject obj) const;
  protected:


}; //IthoRemote
