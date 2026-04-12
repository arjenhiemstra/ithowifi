#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_core_dump.h"
#include "sdkconfig.h"

#include <Arduino.h>
#include <Ticker.h>
#include <Update.h>
#include "esp_wifi.h"

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "websocket.h"
#include "esp32_repartition.h"

#include "tasks/task_mqtt.h"

#include "api/ApiResponse.h"
#include "api/AsyncWebServerAdapter.h"

// #include "LittleFS.h"

// globals
extern TaskHandle_t xTaskWebHandle;
extern uint32_t TaskWebHWmark;
extern bool sysStatReq;
extern bool webauth_ok;
extern bool onOTA;

extern AsyncWebServer server;

#define MQTT_BUFFER_SIZE 5120

extern int MQTT_conn_state;
extern int MQTT_conn_state_new;
extern bool dontReconnectMQTT;
extern bool updateMQTTihtoStatus;
extern unsigned long lastMQTTReconnectAttempt;
extern char modestate[32];

void startTaskWeb();
void TaskWeb(void *pvParameters);
void execWebTasks();

void webServerInit();
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 0)
/* CONFIG_LWIP_IPV4 was introduced in IDF v5.1, set CONFIG_LWIP_IPV4 to 1 by default for IDF v5.0 */
#ifndef CONFIG_LWIP_IPV4
#define CONFIG_LWIP_IPV4 1
#endif // CONFIG_LWIP_IPV4
#endif // ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 0)
#include "handlers/WebServerHandlers.h"
#include "api/WebAPI.h"
#include "handlers/FileHandlers.h"
#include "handlers/WebSocketHandlers.h"
