#include "notifyClients.h"

#define LOG_BUF_SIZE 128

AsyncWebSocket ws("/ws");
unsigned long LastotaWsUpdate = 0;
size_t content_len = 0;

void notifyClients(AsyncWebSocketMessageBuffer* message) {
#if defined (__HW_VERSION_TWO__)
  yield();
  if (xSemaphoreTake(mutexWSsend, (TickType_t) 100 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

    ws.textAll(message);

#if defined (__HW_VERSION_TWO__)
    xSemaphoreGive(mutexWSsend);
  }
#endif

}

void notifyClients(const char * message, size_t len) {

  AsyncWebSocketMessageBuffer * WSBuffer = ws.makeBuffer((uint8_t *)message, len);
  notifyClients(WSBuffer);

}

void jsonSysmessage(const char * id, const char * message) {
  char logBuff[LOG_BUF_SIZE] = "";
  StaticJsonDocument<500> root;

  JsonObject systemstat = root.createNestedObject("sysmessage");
  systemstat["id"] = id;
  systemstat["message"] = message;

  strcpy(logBuff, "");
  char buffer[500];
  size_t len = serializeJson(root, buffer);
  notifyClients(buffer, len);
}


void jsonLogMessage(const __FlashStringHelper * str, logtype type) {
  if (!str) return;
  int length = strlen_P((PGM_P)str);
  if (length == 0) return;
#if defined (__HW_VERSION_ONE__)
  if (length < 400) length = 400;
  char message[400 + 1] = "";
  strncat_P(message, (PGM_P)str, length);
  jsonLogMessage(message, type);
#else
  jsonLogMessage((PGM_P)str, type);
#endif
}

void jsonLogMessage(const char* message, logtype type) {
#if defined (__HW_VERSION_TWO__)
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

    char buffer[512];
    size_t len = serializeJson(root, buffer);


    notifyClients(buffer, len);

#if defined (__HW_VERSION_TWO__)
    xSemaphoreGive(mutexJSONLog);
  }
#endif
}

void jsonLogMessage(JsonObject obj, logtype type) {
#if defined (__HW_VERSION_TWO__)
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

    char buffer[512];
    size_t len = serializeJson(root, buffer);


    notifyClients(buffer, len);

#if defined (__HW_VERSION_TWO__)
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

    char buffer[256];
    size_t len = serializeJson(root, buffer);

    notifyClients(buffer, len);

#if defined (ENABLE_SERIAL)
    printf("OTA Progress: %d%%\n", newPercent);
#endif
  }
}
