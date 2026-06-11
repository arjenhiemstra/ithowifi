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
// Index of the first bi-directional Send remote in the configured slots,
// or -1 if none qualifies. Bi-directional Send remotes are the only
// remote-slot kind that holds a valid sourceID + destinationID pair joined
// to the Itho unit, so they're the only ones from which a 31DA/31D9 RF
// status request can be expected to elicit an answer.
int findFirstBidirectionalSendRemote();
// Send a 31DA + 31D9 status request over RF, addressed from the given
// remote slot's sourceID to its destinationID. The unit answers when
// the slot is bi-directional and joined; non-bidirectional slots are a
// no-op as far as the Itho is concerned. Caller is responsible for
// checking the slot is configured the way they expect — this function
// only guards against an out-of-range index.
//
// If destOverride is non-null it points to a 3-byte address that
// temporarily replaces the slot's destinationID for this one paired
// request — useful for the debug page where the user wants to probe a
// specific Itho's address without having to actually join the remote.
// The slot's stored destinationID is restored after the request.
void sendRFStatusRequest(uint8_t remote_index, const uint8_t *destOverride = nullptr);

// Same as sendRFStatusRequest but only the 31DA half. Used by the
// debug-page button so a user can probe one opcode at a time.
void sendRF31DARequest(uint8_t remote_index, const uint8_t *destOverride = nullptr);

// Same as sendRFStatusRequest but only the 31D9 half. Used by the
// debug-page button so a user can probe one opcode at a time.
void sendRF31D9Request(uint8_t remote_index, const uint8_t *destOverride = nullptr);
// Best-effort lookup of the unit's current FanInfo. Checks ithoMeasurements
// (I2C 31DA) first, then sniffed RF 31DA across active+tracked rfStatusSources.
// Returns "auto"/"low"/"medium"/... or nullptr if no FanInfo data available.
const char *getCurrentFanInfo();
bool fanIsInAuto();
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
