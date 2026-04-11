#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ESPmDNS.h>
#include <string>

#include "IthoQueue.h"
#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "ithodevice/IthoDevice.h"
#include "enum.h"

#include "LittleFS.h"

// globals
extern const char *espName;
inline uint8_t itho_init_status = 0;

const char *hostName();
uint8_t getIthoStatusJSON(JsonObject root);
uint8_t getRFStatusJSON(JsonObject root, int sourceIndex = -1, bool trackedOnly = false);
void getRFStatusConfigJSON(JsonObject root, int sourceIndex = -1);
bool ithoStatusReady();
void getRemotesInfoJSON(JsonObject root);
void getRFDevicesForHADiscJSON(JsonObject root);
void getVirtualRemotesForHADiscJSON(JsonObject root);
void getDeviceInfoJSON(JsonObject root);
void getIthoSettingsBackupJSON(JsonObject root);
bool ithoExecCommand(const char *command, cmdOrigin origin);
bool ithoExecRFCommand(uint8_t remote_index, const char *command, cmdOrigin origin);
bool ithoSendRFCO2(uint8_t remote_index, uint16_t co2level, cmdOrigin origin);
bool ithoSendRFDemand(uint8_t remote_index, uint8_t demand, uint8_t zone, cmdOrigin origin);
bool ithoSetSpeed(const char *speed, cmdOrigin origin);
bool ithoSetSpeed(uint16_t speed, cmdOrigin origin);
bool ithoSetTimer(const char *timer, cmdOrigin origin);
bool ithoSetTimer(uint16_t timer, cmdOrigin origin);
bool ithoSetSpeedTimer(const char *speed, const char *timer, cmdOrigin origin);
bool ithoSetSpeedTimer(uint16_t speed, uint16_t timer, cmdOrigin origin);
void logLastCommand(const char *command, cmdOrigin origin);
void logLastCommand(const char *command, const char *source);
void getLastCMDinfoJSON(JsonObject root);
void updateItho();
void addToQueue();
void setRFdebugLevel(uint8_t level);
double round(float value, int precision);
char toHex(uint8_t c);
std::vector<int> parseHexString(const std::string &input);
void checkFirmwareUpdate();
bool isPrereleaseVersion(const char *version);
const char *getUpdateChannel();
int compareVersions(const std::string &v1, const std::string &v2);
void triggerOTAUpdate(bool beta = false);
void triggerOTAUpdateFromURL(const char *url);
bool performOTAUpdate(bool beta = false);
bool performOTAUpdateFromURL(const char *url);
extern volatile int otaUpdateProgress;
extern volatile bool otaUpdateRequested;
extern volatile bool otaUpdateBeta;
extern char otaUpdateURL[160];
