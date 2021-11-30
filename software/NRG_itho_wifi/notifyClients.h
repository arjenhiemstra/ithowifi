#pragma once

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern AsyncWebSocket ws;
extern SemaphoreHandle_t mutexJSONLog;
extern SemaphoreHandle_t mutexWSsend;

typedef enum { WEBINTERFACE, RFLOG, ITHOSETTINGS } logtype;
extern size_t content_len;

void notifyClients(AsyncWebSocketMessageBuffer* message);

void notifyClients(const char * message, size_t len);
void notifyClients(JsonObjectConst obj);

void jsonSysmessage(const char * id, const char * message);
void logMessagejson(const char* message, logtype type);
void logMessagejson(JsonObject obj, logtype type);
void otaWSupdate(size_t prg, size_t sz);
