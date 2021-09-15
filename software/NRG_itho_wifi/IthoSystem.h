#pragma once

#include <stdio.h>
#include <string.h>
#include <vector>
#include <Arduino.h>

extern size_t itho2401len;


extern uint8_t ithoDeviceID;
extern uint8_t itho_fwversion;
extern volatile uint16_t ithoCurrentVal;
extern uint8_t id0, id1, id2;
extern int8_t swap;
extern struct ihtoDeviceType* ithoSettingsptr;
extern int ithoSettingsLength;

extern bool sendI2CButton;
extern bool sendI2CJoin;
extern bool sendI2CLeave;
extern bool sendI2CDevicetype;
extern bool sendI2CStatusFormat;
extern bool sendI2CStatus;
extern bool send31DA;
extern bool send31D9;
extern bool send2400;
extern bool send2401;
extern bool get2410;
extern bool set2410;
extern bool buttonResult;
extern uint8_t buttonValue;
extern uint8_t index2410;
extern int32_t value2410;
extern int32_t *resultPtr2410;
extern bool i2c_result_updateweb;

extern bool itho_internal_hum_temp;
extern bool temp_hum_updated;
extern float ithoHum;
extern float ithoTemp;

struct ihtoDeviceStatus {
  std::string name;
  enum { is_byte, is_int, is_uint, is_float } type;
  uint8_t length;
  union {
    byte byteval;
    int32_t intval;
    uint32_t uintval;
    float floatval;
  } value;
  uint32_t divider;
};

extern std::vector<ihtoDeviceStatus> ithoStatus;

char* getIthoType(uint8_t deviceID);
int getSettingsLength(uint8_t deviceID, uint8_t version);
void getSetting(uint8_t i, bool updateState, bool loop = false);
void getSetting(uint8_t i, bool updateState, bool loop, struct ihtoDeviceType* getSettingsPtr, uint8_t version);
void getSatusLabel(uint8_t i, struct ihtoDeviceType* getStatusPtr, uint8_t version, char* fStringBuf) ;
void updateSetting(uint8_t i, int32_t value);
struct ihtoDeviceType* getSettingsPtr(uint8_t deviceID, uint8_t version);

uint8_t checksum(const uint8_t* buf, size_t buflen);
void sendButton(uint8_t number, bool i2c_result_updateweb);
void sendJoinI2C(bool i2c_result_updateweb);
void sendLeaveI2C(bool i2c_result_updateweb);
void sendQueryDevicetype(bool i2c_result_updateweb);
void sendQueryStatusFormat(bool i2c_result_updateweb);
void sendQueryStatus(bool i2c_result_updateweb);
void sendQuery2400(bool i2c_result_updateweb);
void sendQuery2401(bool i2c_result_updateweb);
void sendQuery31DA(bool i2c_result_updateweb);
void sendQuery31D9(bool i2c_result_updateweb);
int32_t * sendQuery2410(bool i2c_result_updateweb);
void setSetting2410(bool i2c_result_updateweb);
int quick_pow10(int n);
std::string i2cbuf2string(const uint8_t* data, size_t len);
