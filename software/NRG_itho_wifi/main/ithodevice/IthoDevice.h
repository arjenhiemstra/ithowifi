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

#include "cc1101/IthoPacket.h"
#include "config/IthoRemote.h"
#include "config/SystemConfig.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "i2c_esp32.h"
#include "i2c_logger.h"
#include "tasks/task_mqtt.h"
#include "IthoQueue.h"
#include "enum.h"
#include "tasks/task_syscontrol.h"

struct ithoLabels;

struct ihtoDeviceType
{
  uint8_t DG; // device group?
  uint8_t ID;
  const char *name;
  const uint16_t **settingsMapping;
  uint16_t versionsMapLen;
  const char **settingsDescriptions;
  const uint8_t **statusLabelMapping;
  uint8_t statusMapLen;
  const struct ithoLabels *settingsStatusLabels;
};

extern const struct ihtoDeviceType ithoDevices[];
extern const size_t ithoDevicesLength;

// globals
extern uint8_t ithoDeviceGroup;
extern uint8_t ithoDeviceID;
extern uint8_t itho_hwversion;
extern uint8_t itho_fwversion;
extern int16_t ithoSettingsLength;
extern int16_t ithoStatusLabelLength;
extern volatile uint16_t ithoCurrentVal;
extern const struct ihtoDeviceType *ithoDeviceptr;

// extern bool get2410;
// extern bool set2410;
// extern uint8_t index2410;
// extern int32_t value2410;
extern int32_t *resultPtr2410;
extern bool i2c_result_updateweb;
extern bool i2c_31d9_done;

extern bool itho_internal_hum_temp;
extern float ithoHum;
extern float ithoTemp;

struct ithoDeviceStatus
{
  const char *name;
  enum : uint8_t
  {
    is_byte,
    is_int,
    is_float,
    is_string
  } type;
  uint8_t length;
  union
  {
    byte byteval;
    int32_t intval;
    float floatval;
    const char *stringval;
  } value;
  uint32_t divider;
  uint8_t updated;
  bool is_signed;
  ithoDeviceStatus() : updated(0) {};
};

struct ithoDeviceMeasurements
{
  const char *name;
  enum : uint8_t
  {
    is_int,
    is_float,
    is_string
  } type;
  union
  {
    int32_t intval;
    float floatval;
    const char *stringval;
  } value;
  uint8_t updated;
};

extern std::vector<ithoDeviceStatus> ithoStatus;
extern std::vector<ithoDeviceMeasurements> ithoMeasurements;
extern std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;
extern std::vector<ithoDeviceMeasurements> ithoCounters;

#define MAX_RF_STATUS_SOURCES 20

struct rfStatusSource
{
  uint8_t id[3]{};
  time_t lastSeen{};
  std::vector<ithoDeviceMeasurements> measurements31DA;
  std::vector<ithoDeviceMeasurements> measurements31D9;
  bool active{false};
  bool tracked{false};
  char name[32]{};
};

extern rfStatusSource rfStatusSources[MAX_RF_STATUS_SOURCES];
extern volatile int rfSelectedSourceForParsing;
rfStatusSource *findOrAllocRFStatusSource(uint8_t b0, uint8_t b1, uint8_t b2);

struct lastCommand
{
  char source[30];
  char command[32];
  time_t timestamp;
};

extern struct lastCommand lastCmd;

extern const std::map<cmdOrigin, const char *> cmdOriginMap;

struct ithoSettings
{
  enum : uint8_t
  {
    is_int,
    is_float,
    is_unknown
  } type{is_unknown};
  bool is_signed;
  int32_t value{0};
  uint8_t length{0};
  uint32_t divider;
};

extern ithoSettings *ithoSettingsArray;

const char *getIthoType();
int currentIthoDeviceGroup();
int currentIthoDeviceID();
uint8_t currentItho_hwversion();
uint8_t currentItho_fwversion();
uint16_t currentIthoSettingsLength();
int16_t currentIthoStatusLabelLength();
int getSettingsLength(const uint8_t deviceGroup, const uint8_t deviceID, const uint8_t version);
const char *getSettingLabel(const uint8_t index);
void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop = false);
void processSettingResult(const uint8_t index, const bool loop);
const char *getSpeedLabel();
int getStatusLabelLength(const uint8_t deviceGroup, const uint8_t deviceID, const uint8_t version);
const char *getStatusLabel(const uint8_t i, const struct ihtoDeviceType *statusPtr);
void updateSetting(const uint8_t i, const int32_t value, bool webupdate);
const struct ihtoDeviceType *getDevicePtr(const uint8_t deviceGroup, const uint8_t deviceID);

uint8_t checksum(const uint8_t *buf, size_t buflen);
void sendI2CPWMinit();
void sendCO2init();
void sendCO2query(bool updateweb);
void sendCO2speed(uint8_t speed1 = 0, uint8_t speed2 = 0);
void sendCO2value(uint16_t value);
void sendRemoteCmd(const uint8_t remoteIndex, const IthoCommand command);
void sendQueryDevicetype(bool updateweb);
void sendQueryStatusFormat(bool updateweb);
void sendQueryStatus(bool updateweb);
void sendQuery31DA(bool updateweb);
size_t sendQuery31DA(uint8_t *receive_buffer);
void sendQuery31D9(bool updateweb);
size_t sendQuery31D9(uint8_t *receive_buffer);
void setSettingCE30(uint16_t temporary_temperature, uint16_t fallback_temperature, uint32_t timestamp, bool updateweb);
void setSetting4030(uint16_t index, uint8_t datatype, uint16_t value, uint8_t checked, bool updateweb);
int32_t *sendQuery2410(uint8_t index, bool updateweb);
bool decodeQuery2410(int32_t *, ithoSettings *, double *, double *, double *);
bool setSetting2410(uint8_t index, int32_t value, bool updateweb);
// void setSetting2410(bool updateweb);
void sendQueryCounters(bool updateweb);
void parseRF31DA(const uint8_t *payload, uint8_t len, uint8_t srcId0, uint8_t srcId1, uint8_t srcId2);
void parseRF31D9(const uint8_t *payload, uint8_t len, uint8_t srcId0, uint8_t srcId1, uint8_t srcId2);
bool saveRFTrackedSources();
bool loadRFTrackedSources();
void filterReset();
void IthoPWMcommand(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT);
size_t sendI2cQuery(uint8_t *i2c_command, size_t i2c_command_length, uint8_t *receive_buffer, i2c_cmdref_t origin);
int quickPow10(int n);
std::string i2cBufToString(const uint8_t *data, size_t len);
bool checkI2cReply(const uint8_t *buf, size_t buflen, const uint16_t opcode);
int castToSignedInt(int val, int length);
int64_t castRawBytesToInt(int32_t *valptr, int length, bool is_signed);
uint32_t getDividerFromDatatype(int8_t datatype);
uint8_t getLengthFromDatatype(int8_t datatype);
bool getSignedFromDatatype(int8_t datatype);
