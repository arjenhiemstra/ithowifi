#include <Arduino.h> //fix to prevent INADDR_NONE preprocessor issues
#include "notifyClients.h"


#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
struct mg_mgr mgr;
const char *s_listen_on_ws = "ws://0.0.0.0:8000";
const char *s_listen_on_http = "http://0.0.0.0";
#else
AsyncWebSocket ws("/ws");
#endif

SemaphoreHandle_t mutexJSONLog;
SemaphoreHandle_t mutexWSsend;

unsigned long LastotaWsUpdate = 0;
size_t content_len = 0;

void notifyClients(const char *message)
{
  yield();
  if (xSemaphoreTake(mutexWSsend, (TickType_t)100 / portTICK_PERIOD_MS) == pdTRUE)
  {

#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
    wsSendAll(&mgr, message);
#else
    ws.textAll(message);
#endif

    xSemaphoreGive(mutexWSsend);
  }
}

void notifyClients(JsonObject obj)
{
  size_t len = measureJson(obj);

  char *buffer = new char[len + 1];

  if (buffer)
  {
    serializeJson(obj, buffer, len + 1);
    obj.clear();
    notifyClients(buffer);
  }
  delete[] buffer;
}

void wsSendAll(void *arg, const char *message)
{
#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
  struct mg_mgr *mgr = (struct mg_mgr *)arg;
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
  {
    mg_ws_send(c, message, strlen(message), WEBSOCKET_OP_TEXT);
  }
#else
#endif
}

void jsonSysmessage(const char *id, const char *message)
{
  JsonDocument root;

  JsonObject systemstat = root["sysmessage"].to<JsonObject>();
  systemstat["id"] = id;
  systemstat["message"] = message;

  notifyClients(root.as<JsonObject>());
}

void logMessagejson(const char *message, logtype type)
{
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
  {

    JsonDocument root;
    JsonObject messagebox;

    switch (type)
    {
    case RFLOG:
      messagebox = root["rflog"].to<JsonObject>();
      break;
    case ITHOSETTINGS:
      messagebox = root["ithosettings"].to<JsonObject>();
      break;
    default:
      messagebox = root["messagebox"].to<JsonObject>();
    }

    messagebox["message"] = message;

    notifyClients(root.as<JsonObject>());

    xSemaphoreGive(mutexJSONLog);
  }
}

void logMessagejson(JsonObject obj, logtype type)
{
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
  {

    JsonDocument root;
    JsonObject messagebox = root.to<JsonObject>(); // Fill the object

    switch (type)
    {
    case RFLOG:
      messagebox = root["rflog"].to<JsonObject>();
      break;
    case ITHOSETTINGS:
      messagebox = root["ithosettings"].to<JsonObject>();
      break;
    default:
      messagebox = root["messagebox"].to<JsonObject>();
    }

    for (JsonPair p : obj)
    {
      messagebox[p.key()] = p.value();
    }

    notifyClients(root.as<JsonObject>());

    xSemaphoreGive(mutexJSONLog);
  }
}

void otaWSupdate(size_t prg, size_t sz)
{

  if (millis() - LastotaWsUpdate >= 500)
  { // rate limit messages to twice a second
    LastotaWsUpdate = millis();
    int newPercent = int((prg * 100) / content_len);

    JsonDocument root;
    JsonObject ota = root["ota"].to<JsonObject>();
    ota["progress"] = prg;
    ota["tsize"] = content_len;
    ota["percent"] = newPercent;

    notifyClients(root.as<JsonObject>());

#if defined(ENABLE_SERIAL)
    D_LOG("OTA Progress: %d%%", newPercent);
#endif
  }
}
