#ifndef IthoSystem_h
#define IthoSystem_h

#include <stdio.h>
#include <string.h>
#include <Arduino.h>

extern uint8_t ithoDeviceID;
extern uint8_t itho_fwversion;
extern volatile uint16_t ithoCurrentVal;
extern uint8_t id0, id1, id2;
extern int8_t swap;
extern struct ihtoDeviceType* ithoSettingsptr;
extern int ithoSettingsLength;

char* getIthoType(uint8_t deviceID);
int getSettingsLength(uint8_t deviceID, uint8_t version);
void getSetting(uint8_t i, bool updateState, bool loop = false);
void getSetting(uint8_t i, bool updateState, bool loop, struct ihtoDeviceType* getSettingsPtr, uint8_t version);
void updateSetting(uint8_t i, int32_t value);
struct ihtoDeviceType* getSettingsPtr(uint8_t deviceID, uint8_t version);

uint8_t checksum(const uint8_t* buf, size_t buflen);
void sendButton(uint8_t number);
void sendJoinI2C();
void sendLeaveI2C();
void sendQueryDevicetype();
void sendQueryStatusFormat();
void sendQueryStatus();
void sendQuery2400();
void sendQuery2401();
void sendQuery31DA();
void sendQuery31D9();
int32_t * sendQuery2410(uint8_t settingID, bool webUpdate);
void setSetting2410(uint8_t settingID, int32_t value);

#endif // IthoSystem_h
