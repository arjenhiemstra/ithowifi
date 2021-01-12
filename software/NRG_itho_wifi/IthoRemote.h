#ifndef IthoRemote_h
#define IthoRemote_h

#define MAX_NUMBER_OF_REMOTES 10

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Ticker.h>


class IthoRemote {
  private:
    struct Remote {
      uint8_t ID[8] { 0, 0, 0, 0, 0, 0, 0, 0 };
      char name[32];
      uint8_t button1{ IthoLow };
      uint8_t button2{ IthoMedium };
      uint8_t button3{ IthoHigh };
      uint8_t button4{ IthoTimer1 };
      uint8_t button5{ IthoTimer2 };
      uint8_t button6{ IthoTimer3 };
      uint8_t button7{ IthoJoin };
      uint8_t button8{ IthoLeave };
      void set(JsonObjectConst);
      void get(JsonObject, int index) const;
    };
    enum IthoCommand
    {
      IthoUnknown = 0,

      IthoJoin = 4,
      IthoLeave = 8,

      IthoStandby = 34,
      IthoLow = 35,
      IthoMedium = 36,
      IthoHigh = 37,
      IthoFull = 38,

      IthoTimer1 = 41,
      IthoTimer2 = 51,
      IthoTimer3 = 61,

      //duco c system remote
      DucoStandby = 251,
      DucoLow = 252,
      DucoMedium = 253,
      DucoHigh = 254
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
    void updateRemoteName(uint8_t index, char* remoteName);
    int remoteIndex(int* id);
    int * getRemoteIDbyIndex(int index);
    char * getRemoteNamebyIndex(int index);
    bool checkID(int* id);
    void set(JsonObjectConst);
    void get(JsonObject) const;
  protected:


}; //IthoRemote


#endif //IthoRemote_h
