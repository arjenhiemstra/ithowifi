#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <Arduino.h>
#include <Ticker.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern uint8_t ithoDeviceID;
extern uint8_t itho_fwversion;
extern volatile uint16_t ithoCurrentVal;
extern uint8_t id0, id1, id2;
extern struct ihtoDeviceType* ithoDeviceptr;
extern int ithoSettingsLength;

extern bool sendI2CButton;
extern bool sendI2CTimer;
extern bool sendI2CJoin;
extern bool sendI2CLeave;
extern bool sendI2CDevicetype;
extern bool sendI2CStatusFormat;
extern bool sendI2CStatus;
extern bool send31DA;
extern bool send31D9;
extern bool get2410;
extern bool set2410;
extern bool buttonResult;
extern uint8_t buttonValue;
extern uint8_t timerValue;
extern uint8_t index2410;
extern int32_t value2410;
extern int32_t *resultPtr2410;
extern bool i2c_result_updateweb;

extern bool itho_internal_hum_temp;
extern float ithoHum;
extern float ithoTemp;

extern Ticker getSettingsHack;
extern SemaphoreHandle_t mutexI2Ctask;

enum cmdOrigin {
    UNKNOWN,
    HTMLAPI,
    MQTTAPI,
    REMOTE,
    WEB
};

struct ithoDeviceStatus {
  std::string name;
  enum { is_byte, is_int, is_uint, is_float, is_string } type;
  uint8_t length;
  union {
    byte byteval;
    int32_t intval;
    uint32_t uintval;
    float floatval;
    const char* stringval;
  } value;
  uint32_t divider;
};


struct ithoDeviceMeasurements {
  std::string name;
  enum { is_int, is_float, is_string } type;
  union {
    int32_t intval;
    float floatval;
    const char* stringval;
  } value;
};

extern std::vector<ithoDeviceStatus> ithoStatus;
extern std::vector<ithoDeviceMeasurements> ithoMeasurements;
extern std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;

struct lastCommand {
  const char* source;
  char command[20];
  time_t timestamp;
};

extern struct lastCommand lastCmd;

extern const std::map<cmdOrigin, const char*> cmdOriginMap;


char* getIthoType(const uint8_t deviceID);
int getSettingsLength(const uint8_t deviceID, const uint8_t version);
void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop = false);
void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop, const struct ihtoDeviceType* settingsPtr, const uint8_t deviceID, const uint8_t version);
void getSatusLabel(const uint8_t i, const struct ihtoDeviceType* statusPtr, const uint8_t version, char* fStringBuf) ;
void updateSetting(const uint8_t i, const int32_t value, bool webupdate);
struct ihtoDeviceType* getDevicePtr(uint8_t deviceID);

uint8_t checksum(const uint8_t* buf, size_t buflen);
void sendI2CPWMinit();
void sendButton(uint8_t number, bool &updateweb);
void sendTimer(uint8_t timer, bool &updateweb);
void sendJoinI2C(bool &updateweb);
void sendLeaveI2C(bool &updateweb);
void sendQueryDevicetype(bool &updateweb);
void sendQueryStatusFormat(bool &updateweb);
void sendQueryStatus(bool &updateweb);
void sendQuery31DA(bool &updateweb);
void sendQuery31D9(bool &updateweb);
int32_t * sendQuery2410(bool &updateweb);
void setSetting2410(bool &updateweb);
int quick_pow10(int n);
std::string i2cbuf2string(const uint8_t* data, size_t len);
