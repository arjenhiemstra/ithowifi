#pragma once

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

extern AsyncWebSocket ws;

typedef enum { WEBINTERFACE, RFLOG, ITHOSETTINGS } logtype;
extern size_t content_len;

void notifyClients(AsyncWebSocketMessageBuffer* message);

void notifyClients(const char * message, size_t len);

void jsonSysmessage(const char * id, const char * message);
void jsonLogMessage(const __FlashStringHelper * str, logtype type);
void jsonLogMessage(const char* message, logtype type);
void jsonLogMessage(JsonObject obj, logtype type);
void otaWSupdate(size_t prg, size_t sz);
