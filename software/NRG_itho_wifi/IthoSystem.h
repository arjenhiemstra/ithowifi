#pragma once

#include <cstdio>
#include <string>
#include <cstring>
#include <vector>
#include <map>

#include <Arduino.h>
#include <Ticker.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "IthoPacket.h"
#include "IthoRemote.h"

extern uint8_t ithoDeviceID;
extern uint8_t itho_fwversion;
extern volatile uint16_t ithoCurrentVal;
extern uint8_t id0, id1, id2;
extern const struct ihtoDeviceType* ithoDeviceptr;
extern int16_t ithoSettingsLength;

extern bool get2410;
extern bool set2410;
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
  const char* name;
  enum : uint8_t { is_byte, is_int, is_uint, is_float, is_string } type;
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
  const char* name;
  enum : uint8_t { is_int, is_float, is_string } type;
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

struct ithoSettings {
  enum : uint8_t { is_int8, is_int16, is_int32, is_uint8, is_uint16, is_uint32, is_float2, is_float10, is_float100, is_float1000, is_unknown } type {is_unknown};
  int32_t value {0};
  uint8_t length {0};
};

extern ithoSettings * ithoSettingsArray;



const char* getIthoType(const uint8_t deviceID);
int getSettingsLength(const uint8_t deviceID, const uint8_t version);
void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop = false);
void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop, const struct ihtoDeviceType* settingsPtr, const uint8_t deviceID, const uint8_t version);
int getStatusLabelLength(const uint8_t deviceID, const uint8_t version);
const char* getSatusLabel(const uint8_t i, const struct ihtoDeviceType* statusPtr, const uint8_t version) ;
void updateSetting(const uint8_t i, const int32_t value, bool webupdate);
const struct ihtoDeviceType* getDevicePtr(const uint8_t deviceID);

uint8_t checksum(const uint8_t* buf, size_t buflen);
void sendI2CPWMinit();
void sendRemoteCmd(const uint8_t remoteIndex, const IthoCommand command, IthoRemote &remotes);
void sendQueryDevicetype(bool updateweb);
void sendQueryStatusFormat(bool updateweb);
void sendQueryStatus(bool updateweb);
void sendQuery31DA(bool updateweb);
void sendQuery31D9(bool updateweb);
int32_t * sendQuery2410(bool &updateweb);
void setSetting2410(bool &updateweb);
void filterReset();
int quick_pow10(int n);
std::string i2cbuf2string(const uint8_t* data, size_t len);
