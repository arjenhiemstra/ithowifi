#include "api/WebAPIv2Rest.h"
#include "api/WebAPIv2.h"
#include "config/SystemConfig.h"
#include "globals.h"
#include "generic_functions.h"
#include "IthoQueue.h"
#include "config/IthoRemote.h"
#include "ithodevice/IthoDevice.h"

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

// --- Helpers ---

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

// --- GET endpoints ---

// GET /api/v2/speed
static void handleGetSpeed(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  data["currentspeed"] = ithoCurrentVal;
  sendSuccess(request, data);
}

// GET /api/v2/status
static void handleGetStatus(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  JsonObject obj = data["ithostatus"].to<JsonObject>();
  getIthoStatusJSON(obj);
  sendSuccess(request, data);
}

// GET /api/v2/device
static void handleGetDevice(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  JsonObject obj = data["deviceinfo"].to<JsonObject>();
  getDeviceInfoJSON(obj);
  sendSuccess(request, data);
}

// GET /api/v2/queue
static void handleGetQueue(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  JsonArray arr = data["queue"].to<JsonArray>();
  ithoQueue.get(arr);
  data["ithoSpeed"] = ithoQueue.ithoSpeed;
  data["ithoOldSpeed"] = ithoQueue.ithoOldSpeed;
  data["fallBackSpeed"] = ithoQueue.fallBackSpeed;
  sendSuccess(request, data);
}

// GET /api/v2/lastcmd
static void handleGetLastCmd(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  JsonObject obj = data["lastcmd"].to<JsonObject>();
  getLastCMDinfoJSON(obj);
  sendSuccess(request, data);
}

// GET /api/v2/remotes
static void handleGetRemotes(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  JsonObject obj = data["remotesinfo"].to<JsonObject>();
  getRemotesInfoJSON(obj);
  sendSuccess(request, data);
}

// GET /api/v2/vremotes
static void handleGetVRemotes(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  JsonObject obj = data.as<JsonObject>();
  virtualRemotes.get(obj, "vremotesinfo");
  sendSuccess(request, data);
}

// GET /api/v2/rfstatus?name=X (optional filter)
static void handleGetRFStatus(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;

  JsonDocument data;

  if (request->hasParam("name"))
  {
    const char *nameFilter = request->getParam("name")->value().c_str();
    for (int i = 0; i < MAX_RF_STATUS_SOURCES; i++)
    {
      if (!rfStatusSources[i].active || !rfStatusSources[i].tracked)
        continue;
      if (strcmp(rfStatusSources[i].name, nameFilter) != 0)
        continue;

      JsonObject obj = data["rfstatus"].to<JsonObject>();
      char idStr[10];
      snprintf(idStr, sizeof(idStr), "%02X:%02X:%02X",
               rfStatusSources[i].id[0], rfStatusSources[i].id[1], rfStatusSources[i].id[2]);
      obj["id"] = idStr;
      obj["name"] = rfStatusSources[i].name;
      obj["lastSeen"] = static_cast<int32_t>(rfStatusSources[i].lastSeen);
      JsonObject mdata = obj["data"].to<JsonObject>();
      for (const auto &m : rfStatusSources[i].measurements31D9)
      {
        if (m.type == ithoDeviceMeasurements::is_int)
          mdata[m.name] = m.value.intval;
        else if (m.type == ithoDeviceMeasurements::is_float)
          mdata[m.name] = m.value.floatval;
        else if (m.type == ithoDeviceMeasurements::is_string)
          mdata[m.name] = m.value.stringval;
      }
      for (const auto &m : rfStatusSources[i].measurements31DA)
      {
        if (m.type == ithoDeviceMeasurements::is_int)
          mdata[m.name] = m.value.intval;
        else if (m.type == ithoDeviceMeasurements::is_float)
          mdata[m.name] = m.value.floatval;
        else if (m.type == ithoDeviceMeasurements::is_string)
          mdata[m.name] = m.value.stringval;
      }
      sendSuccess(request, data);
      return;
    }
    sendFail(request, "source not found", 404);
    return;
  }

  JsonObject obj = data["rfstatus"].to<JsonObject>();
  getRFStatusJSON(obj, -1, true);
  sendSuccess(request, data);
}

// GET /api/v2/settings?index=N
static void handleGetSettings(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;

  if (!request->hasParam("index"))
  {
    sendFail(request, "missing required parameter: index");
    return;
  }

  const char *indexStr = request->getParam("index")->value().c_str();
  unsigned long idx;
  try
  {
    size_t pos;
    idx = std::stoul(indexStr, &pos);
    if (pos != strlen(indexStr))
      throw std::invalid_argument("extra characters after index");
  }
  catch (const std::exception &e)
  {
    sendFail(request, "invalid index parameter");
    return;
  }

  if (ithoSettingsArray == nullptr)
  {
    sendFail(request, "settings array not initialized");
    return;
  }

  if (idx >= currentIthoSettingsLength())
  {
    sendFail(request, "index out of range");
    return;
  }

  resultPtr2410 = nullptr;
  auto timeoutmillis = millis() + 3000;
  resultPtr2410 = sendQuery2410(idx, false);

  while (resultPtr2410 == nullptr && millis() < timeoutmillis)
  {
    // wait for result
  }

  ithoSettings *setting = &ithoSettingsArray[idx];
  double cur = 0.0, min = 0.0, max = 0.0;

  if (resultPtr2410 == nullptr)
  {
    sendError(request, "I2C command failed");
    return;
  }

  if (!decodeQuery2410(resultPtr2410, setting, &cur, &min, &max))
  {
    sendError(request, "failed to decode setting value");
    return;
  }

  JsonDocument data;
  data["index"] = idx;
  data["label"] = getSettingLabel(idx);
  data["current"] = cur;
  data["minimum"] = min;
  data["maximum"] = max;
  sendSuccess(request, data);
}

// --- Route registration ---

void registerRestAPIv2Routes(AsyncWebServer &server)
{
  // GET endpoints
  server.on("/api/v2/speed", HTTP_GET, handleGetSpeed);
  server.on("/api/v2/status", HTTP_GET, handleGetStatus);
  server.on("/api/v2/device", HTTP_GET, handleGetDevice);
  server.on("/api/v2/queue", HTTP_GET, handleGetQueue);
  server.on("/api/v2/lastcmd", HTTP_GET, handleGetLastCmd);
  server.on("/api/v2/remotes", HTTP_GET, handleGetRemotes);
  server.on("/api/v2/vremotes", HTTP_GET, handleGetVRemotes);
  server.on("/api/v2/rfstatus", HTTP_GET, handleGetRFStatus);
  server.on("/api/v2/settings", HTTP_GET, handleGetSettings);
}
