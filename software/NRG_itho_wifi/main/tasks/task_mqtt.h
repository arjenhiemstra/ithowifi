#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include <PubSubClient.h>

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"

#include "tasks/task_web.h"

#include "ApiResponse.h"

extern TaskHandle_t xTaskMQTTHandle;
extern uint32_t TaskMQTTHWmark;
extern bool sysStatReq;

#define MQTT_BUFFER_SIZE 5120

extern int MQTT_conn_state;
extern int MQTT_conn_state_new;
extern bool dontReconnectMQTT;
extern bool updateMQTTihtoStatus;
extern bool updateMQTTmodeStatus;
extern PubSubClient mqttClient;
extern Ticker TaskMQTTTimeout;
extern bool sendHomeAssistantDiscovery;
extern bool sendHADiscoveryIthoStatusItems;
extern bool updateIthoMQTT;

void startTaskMQTT();
void TaskMQTT(void *pvParameters);
void execMQTTTasks();
void mqttInit();
void mqttSendStatus();
void mqttSendRemotesInfo();
void mqttPublishLastcmd();
void mqttSendSettingsJSON();
void mqttCallback(const char *topic, const byte *payload, unsigned int length);
void updateState(uint16_t newState);
void mqttHomeAssistantDiscovery();
void HADiscoveryFan();
void HADiscoveryTemperature();
void HADiscoveryHumidity();
void HADiscoveryIthoStatusItems();
void addHADevInfo(JsonObject obj);
void sendHADiscovery(JsonObject obj, const char *topic);
bool setupMQTTClient();
bool reconnect();
bool api_cmd_allowed(const char* cmd);
