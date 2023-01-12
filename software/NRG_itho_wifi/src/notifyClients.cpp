#include <Arduino.h> //fix to prevent INADDR_NONE preprocessor issues
#include "notifyClients.h"

struct mg_mgr mgr;
const char *s_listen_on_ws = "ws://0.0.0.0:8000";
#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
const char *s_listen_on_http = "http://0.0.0.0:8080";
#endif

SemaphoreHandle_t mutexJSONLog;
SemaphoreHandle_t mutexWSsend;

// AsyncWebSocket ws("/ws");

unsigned long LastotaWsUpdate = 0;
size_t content_len = 0;

void notifyClients(const char *message)
{
  yield();
  if (xSemaphoreTake(mutexWSsend, (TickType_t)100 / portTICK_PERIOD_MS) == pdTRUE)
  {

    // ws.textAll(message);
    wsSendAll(&mgr, message);

    xSemaphoreGive(mutexWSsend);
  }
}

void notifyClients(JsonObjectConst obj)
{
  size_t len = measureJson(obj);
  char *buffer = new char[len + 1];
  // AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer)
  {
    serializeJson(obj, buffer, len + 1);
    notifyClients(buffer);
  }
  delete[] buffer;
}

void wsSendAll(void *arg, const char *message)
{
  struct mg_mgr *mgr = (struct mg_mgr *)arg;
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
  {
    mg_ws_send(c, message, strlen(message), WEBSOCKET_OP_TEXT);
  }
}

void jsonSysmessage(const char *id, const char *message)
{
  StaticJsonDocument<500> root;

  JsonObject systemstat = root.createNestedObject("sysmessage");
  systemstat["id"] = id;
  systemstat["message"] = message;

  notifyClients(root.as<JsonObjectConst>());
}

void logMessagejson(const char *message, logtype type)
{
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
  {

    StaticJsonDocument<512> root;
    JsonObject messagebox;

    switch (type)
    {
    case RFLOG:
      messagebox = root.createNestedObject("rflog");
      break;
    case ITHOSETTINGS:
      messagebox = root.createNestedObject("ithosettings");
      break;
    default:
      messagebox = root.createNestedObject("messagebox");
    }

    messagebox["message"] = message;

    notifyClients(root.as<JsonObjectConst>());

    xSemaphoreGive(mutexJSONLog);
  }
}

void logMessagejson(JsonObject obj, logtype type)
{
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
  {

    StaticJsonDocument<512> root;
    JsonObject messagebox = root.to<JsonObject>(); // Fill the object

    switch (type)
    {
    case RFLOG:
      messagebox = root.createNestedObject("rflog");
      break;
    case ITHOSETTINGS:
      messagebox = root.createNestedObject("ithosettings");
      break;
    default:
      messagebox = root.createNestedObject("messagebox");
    }

    for (JsonPair p : obj)
    {
      messagebox[p.key()] = p.value();
    }

    notifyClients(root.as<JsonObjectConst>());

    xSemaphoreGive(mutexJSONLog);
  }
}

void otaWSupdate(size_t prg, size_t sz)
{

  if (millis() - LastotaWsUpdate >= 500)
  { // rate limit messages to twice a second
    LastotaWsUpdate = millis();
    int newPercent = int((prg * 100) / content_len);

    StaticJsonDocument<256> root;
    JsonObject ota = root.createNestedObject("ota");
    ota["progress"] = prg;
    ota["tsize"] = content_len;
    ota["percent"] = newPercent;

    notifyClients(root.as<JsonObjectConst>());

#if defined(ENABLE_SERIAL)
    D_LOG("OTA Progress: %d%%", newPercent);
#endif
  }
}
