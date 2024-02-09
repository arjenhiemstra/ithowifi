#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_core_dump.h"
#include "sdkconfig.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <mongoose.h>
#include "esp_wifi.h"
#include <SPIFFSEditor.h>

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "websocket.h"
#include "esp32_repartition.h"

#include "tasks/task_mqtt.h"

#include "ApiResponse.h"
#include "AsyncWebServerAdapter.h"

#include "LittleFS.h"

// globals
extern TaskHandle_t xTaskWebHandle;
extern uint32_t TaskWebHWmark;
extern bool sysStatReq;
extern bool webauth_ok;
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

void ArduinoOTAinit();
void webServerInit();
void MDNSinit();

void handleAPI(AsyncWebServerRequest *request);
void handleAPI_new(AsyncWebServerRequest *request);
ApiResponse::api_response_status_t checkAuthenticationAsyncWeb(AsyncWebServerRequest *request);

// void handleCoreCrash(AsyncWebServerRequest *request);
void handleCoredumpDownload(AsyncWebServerRequest *request);
void handleCurLogDownload(AsyncWebServerRequest *request);
void handlePrevLogDownload(AsyncWebServerRequest *request);
void handleFileCreate(AsyncWebServerRequest *request);
void handleFileDelete(AsyncWebServerRequest *request);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
bool handleFileRead(AsyncWebServerRequest *request);
void handleStatus(AsyncWebServerRequest *request);
void handleFileList(AsyncWebServerRequest *request);
bool prevlog_available();
void jsonSystemstat();
const char *getContentType(bool download, const char *filename);
int my_stat(const char *path, size_t *size, time_t *mtime);
void httpEvent(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
void mg_serve_fs(struct mg_connection *c, void *ev_data, bool download);
void mg_handleFileList(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
bool mg_handleFileRead(struct mg_connection *c, void *ev_data, bool download);
void mg_handleStatus(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
void mg_handleFileDelete(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
void mg_handleFileCreate(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
