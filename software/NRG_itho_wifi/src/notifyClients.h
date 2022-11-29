#pragma once

#include <cstdio>
#include <string>

#include "mongoose.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sys_log.h"

extern struct mg_mgr mgr;
extern const char *s_listen_on_ws;
extern const char *s_listen_on_http;

// extern AsyncWebSocket ws;
extern SemaphoreHandle_t mutexJSONLog;
extern SemaphoreHandle_t mutexWSsend;

typedef enum
{
    WEBINTERFACE,
    RFLOG,
    ITHOSETTINGS
} logtype;

typedef enum
{
    WEB_UPDATE_FALSE = 0x0,
    WEB_UPDATE_TRUE = 0x01,
    SETTINGS_FIRST_RUN = 0x02,
    SETTINGS_UPDATE_TABLE = 0x03,
    SETTINGS_UPDATE_DEBUG = 0x04
} web_status_update_t;

extern size_t content_len;

void notifyClients(const char *message);
void notifyClients(JsonObjectConst obj);
void wsSendAll(void *arg, const char *message);

void jsonSysmessage(const char *id, const char *message);
void logMessagejson(const char *message, logtype type);
void logMessagejson(JsonObject obj, logtype type);
void otaWSupdate(size_t prg, size_t sz);
