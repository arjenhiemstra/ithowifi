#pragma once

#define LOG_BUF_SIZE 128

#include <cstdio>
#include <string>

#include "mongoose.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Dbglog.h"

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
extern size_t content_len;

void notifyClients(const char *message);
void notifyClients(JsonObjectConst obj);
void wsSendAll(void *arg, const char *message);

void jsonSysmessage(const char *id, const char *message);
void logMessagejson(const char *message, logtype type);
void logMessagejson(JsonObject obj, logtype type);
void otaWSupdate(size_t prg, size_t sz);
