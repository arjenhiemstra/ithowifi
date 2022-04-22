#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <mongoose.h>
#include "esp_wifi.h"
#include <SPIFFSEditor.h>

#include "SystemConfig.h"
#include "WifiConfig.h"
#include "hardware.h"
#include "globals.h"
#include "Dbglog.h"
#include "flashLog.h"
#include "notifyClients.h"
#include "websocket.h"

#include "task_mqtt.h"

// globals
extern TaskHandle_t xTaskWebHandle;
extern uint32_t TaskWebHWmark;
extern bool sysStatReq;

extern AsyncWebServer server;

#define MQTT_BUFFER_SIZE 5120

extern int MQTT_conn_state;
extern int MQTT_conn_state_new;
extern bool dontReconnectMQTT;
extern bool updateMQTTihtoStatus;
extern unsigned long lastMQTTReconnectAttempt;

void startTaskWeb();
void TaskWeb(void *pvParameters);
void execWebTasks();

void ArduinoOTAinit();
void webServerInit();
void MDNSinit();

void handleAPI(AsyncWebServerRequest *request);
void handleDebug(AsyncWebServerRequest *request);
void handleCurLogDownload(AsyncWebServerRequest *request);
void handlePrevLogDownload(AsyncWebServerRequest *request);

void jsonSystemstat();
