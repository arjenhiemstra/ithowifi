#include "notifyClients.h"

#define LOG_BUF_SIZE 128

AsyncWebSocket ws("/ws");
unsigned long LastotaWsUpdate = 0;
size_t content_len = 0;

void notifyClients(AsyncWebSocketMessageBuffer* message) {
#if defined (HW_VERSION_TWO)
  yield();
  if (xSemaphoreTake(mutexWSsend, (TickType_t) 100 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

    ws.textAll(message);

#if defined (HW_VERSION_TWO)
    xSemaphoreGive(mutexWSsend);
  }
#endif

}

void notifyClients(const char * message, size_t len) {

  AsyncWebSocketMessageBuffer * WSBuffer = ws.makeBuffer((uint8_t *)message, len);
  notifyClients(WSBuffer);

}

void notifyClients(JsonObjectConst obj) {
  size_t len = measureJson(obj);
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    serializeJson(obj, (char *)buffer->get(), len + 1);
    notifyClients(buffer);
  }  
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


void logMessagejson(const __FlashStringHelper * str, logtype type) {
  if (!str) return;
  int length = strlen_P((PGM_P)str);
  if (length == 0) return;
#if defined (HW_VERSION_ONE)
  if (length < 400) length = 400;
  char message[400 + 1] = "";
  strncat_P(message, (PGM_P)str, length);
  logMessagejson(message, type);
#else
  logMessagejson((PGM_P)str, type);
#endif
}

void logMessagejson(const char* message, logtype type) {
#if defined (HW_VERSION_TWO)
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

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

#if defined (HW_VERSION_TWO)
    xSemaphoreGive(mutexJSONLog);
  }
#endif
}

void logMessagejson(JsonObject obj, logtype type) {
#if defined (HW_VERSION_TWO)
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

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

#if defined (HW_VERSION_TWO)
    xSemaphoreGive(mutexJSONLog);
  }
#endif
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
