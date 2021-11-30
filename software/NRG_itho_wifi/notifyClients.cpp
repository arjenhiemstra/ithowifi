#include "notifyClients.h"

#define LOG_BUF_SIZE 128

SemaphoreHandle_t mutexJSONLog;
SemaphoreHandle_t mutexWSsend;

AsyncWebSocket ws("/ws");
unsigned long LastotaWsUpdate = 0;
size_t content_len = 0;

void notifyClients(AsyncWebSocketMessageBuffer* message) {
  yield();
  if (xSemaphoreTake(mutexWSsend, (TickType_t) 100 / portTICK_PERIOD_MS) == pdTRUE) {

    ws.textAll(message);

    xSemaphoreGive(mutexWSsend);
  }

}

void notifyClients(const char * message, size_t len) {
//  char* buffer = new char[len + 1];
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer((uint8_t *)message, len);
  notifyClients(buffer);
  //delete[] buffer;
}

void notifyClients(JsonObjectConst obj) {
  size_t len = measureJson(obj);
//  char* buffer = new char[len + 1];  
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    serializeJson(obj, (char *)buffer->get(), len + 1);
    notifyClients(buffer);
  }
  //delete[] buffer;
  
}

void jsonSysmessage(const char * id, const char * message) {
  char logBuff[LOG_BUF_SIZE] = "";
  StaticJsonDocument<500> root;

  JsonObject systemstat = root.createNestedObject("sysmessage");
  systemstat["id"] = id;
  systemstat["message"] = message;

  strcpy(logBuff, "");

  notifyClients(root.as<JsonObjectConst>());

}

void logMessagejson(const char* message, logtype type) {
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {

    StaticJsonDocument<512> root;
    JsonObject messagebox;

    switch (type) {
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

void logMessagejson(JsonObject obj, logtype type) {
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {

    StaticJsonDocument<512> root;
    JsonObject messagebox = root.to<JsonObject>(); // Fill the object

    switch (type) {
      case RFLOG:
        messagebox = root.createNestedObject("rflog");
        break;
      case ITHOSETTINGS:
        messagebox = root.createNestedObject("ithosettings");
        break;
      default:
        messagebox = root.createNestedObject("messagebox");
    }

    for (JsonPair p : obj) {
      messagebox[p.key()] = p.value();
    }

    notifyClients(root.as<JsonObjectConst>());

    xSemaphoreGive(mutexJSONLog);
  }
}

void otaWSupdate(size_t prg, size_t sz) {

  if (millis() - LastotaWsUpdate >= 500) { //rate limit messages to twice a second
    LastotaWsUpdate = millis();
    int newPercent = int((prg * 100) / content_len);

    StaticJsonDocument<256> root;
    JsonObject ota = root.createNestedObject("ota");
    ota["progress"] = prg;
    ota["tsize"] = content_len;
    ota["percent"] = newPercent;

    notifyClients(root.as<JsonObjectConst>());

#if defined (ENABLE_SERIAL)
    D_LOG("OTA Progress: %d%%\n", newPercent);
#endif
  }
}
