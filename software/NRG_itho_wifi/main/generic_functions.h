#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <string>

#include "IthoQueue.h"
#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "IthoSystem.h"
#include "enum.h"

#include "LittleFS.h"

// globals

const char *hostName();
void getIthoStatusJSON(JsonObject root);
void getRemotesInfoJSON(JsonObject root);
void getIthoSettingsBackupJSON(JsonObject root);
void getIthoSettingsBackupJSONPlus(JsonArray sumJson);
bool ithoExecCommand(const char *command, cmdOrigin origin);
bool ithoExecRFCommand(uint8_t remote_index, const char *command, cmdOrigin origin);
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
void add2queue();
void setRFdebugLevel(uint8_t level);
double round(double value, int precision);
