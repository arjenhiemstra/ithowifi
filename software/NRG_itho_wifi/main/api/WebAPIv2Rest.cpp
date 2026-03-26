#include "api/WebAPIv2Rest.h"
#include "api/WebAPIv2.h"
#include "config/SystemConfig.h"
#include "globals.h"

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

static bool checkRestAuth(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_api)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->requestAuthentication();
      return false;
    }
  }
  return true;
}

static void sendSuccess(AsyncWebServerRequest *request, JsonDocument &data, int statusCode = 200)
{
  JsonDocument doc;
  doc["status"] = "success";
  doc["data"] = data;
  time_t now;
  time(&now);
  doc["data"]["timestamp"] = now;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(statusCode);
  serializeJson(doc, *response);
  request->send(response);
}

static void sendFail(AsyncWebServerRequest *request, const char *reason, int statusCode = 400)
{
  JsonDocument doc;
  doc["status"] = "fail";
  time_t now;
  time(&now);
  doc["data"]["timestamp"] = now;
  doc["data"]["failreason"] = reason;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(statusCode);
  serializeJson(doc, *response);
  request->send(response);
}

static void sendError(AsyncWebServerRequest *request, const char *message, int statusCode = 500)
{
  JsonDocument doc;
  doc["status"] = "error";
  doc["message"] = message;
  time_t now;
  time(&now);
  doc["timestamp"] = now;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(statusCode);
  serializeJson(doc, *response);
  request->send(response);
}

// GET /api/v2/speed
static void handleGetSpeed(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  data["currentspeed"] = ithoCurrentVal;
  sendSuccess(request, data);
}

void registerRestAPIv2Routes(AsyncWebServer &server)
{
  // GET endpoints
  server.on("/api/v2/speed", HTTP_GET, handleGetSpeed);
}
