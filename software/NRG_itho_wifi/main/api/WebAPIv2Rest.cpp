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
  response->addHeader("Access-Control-Allow-Origin", "*");
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
  response->addHeader("Access-Control-Allow-Origin", "*");
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
  response->addHeader("Access-Control-Allow-Origin", "*");
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

// GET /api/v2/ota
// Combined firmware info + OTA progress for HA / other integrations.
static void handleGetOTA(AsyncWebServerRequest *request)
{
  if (!checkRestAuth(request))
    return;
  JsonDocument data;
  JsonObject obj = data["ota"].to<JsonObject>();
  obj["installed_version"] = fw_version;
  obj["latest_fw"] = firmwareInfo.latest_fw;
  obj["latest_beta_fw"] = firmwareInfo.latest_beta_fw;
  obj["fw_update_available"] = firmwareInfo.fw_update_available;

  int p = otaUpdateProgress;
  const char *state;
  if (p == -1)
    state = "idle";
  else if (p == -2)
    state = "error";
  else if (p == 101)
    state = "done";
  else if (p == 0)
    state = "starting";
  else
    state = "downloading";
  obj["state"] = state;
  obj["progress"] = (p >= 0 && p <= 100) ? p : 0;
  obj["raw"] = p;
  sendSuccess(request, data);
}

// POST /api/v2/ota
// Body: {"channel":"stable"} | {"channel":"beta"} | {"url":"https://github.com/arjenhiemstra/ithowifi/..."}
static void handlePostOTA(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();

  if (!body["url"].isNull())
  {
    const char *url = body["url"] | "";
    if (strncmp(url, "https://github.com/arjenhiemstra/ithowifi/", 42) != 0)
    {
      sendFail(request, "invalid firmware URL");
      return;
    }
    JsonDocument data;
    data["result"] = "firmware update started";
    data["url"] = url;
    sendSuccess(request, data);
    triggerOTAUpdateFromURL(url);
    return;
  }

  if (!body["channel"].isNull())
  {
    const char *channel = body["channel"] | "";
    bool beta = (strcmp(channel, "beta") == 0);
    if (!beta && strcmp(channel, "stable") != 0)
    {
      sendFail(request, "invalid channel - must be 'stable' or 'beta'");
      return;
    }
    if (strlen(beta ? firmwareInfo.link_beta : firmwareInfo.link) == 0)
    {
      sendFail(request, "no firmware URL available - run firmware check first or check internet connection");
      return;
    }
    JsonDocument data;
    data["result"] = "firmware update started";
    data["url"] = beta ? firmwareInfo.link_beta : firmwareInfo.link;
    data["version"] = beta ? firmwareInfo.latest_beta_fw : firmwareInfo.latest_fw;
    sendSuccess(request, data);
    triggerOTAUpdate(beta);
    return;
  }

  sendFail(request, "missing required field: channel or url");
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
  // Full remote config (new format)
  JsonObject obj = data.to<JsonObject>();
  remotes.get(obj, "remotes");
  // Legacy capabilities (backward compat)
  JsonObject legacy = data["remotesinfo"].to<JsonObject>();
  getRemotesInfoJSON(legacy);
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

// --- POST/PUT endpoint handlers ---

// POST /api/v2/command
// Body: {"command":"low"} or {"speed":100} or {"speed":100,"timer":10}
static void handlePostCommand(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();

  // Handle percentage (0-100) and fandemand (0-200)
  // RF CO2 mode: auto command + ventilation demand
  // PWM2I2C mode: mapped to speed (0-255)
  if (!body["percentage"].isNull() || !body["fandemand"].isNull())
  {
    int demand = 0;
    if (!body["percentage"].isNull())
    {
      int pct = body["percentage"].as<int>();
      if (pct < 0 || pct > 100)
      {
        sendFail(request, "percentage out of range (0-100)");
        return;
      }
      demand = pct * 2; // 0-100% → 0-200 demand
    }
    else
    {
      demand = body["fandemand"].as<int>();
      if (demand < 0 || demand > 200)
      {
        sendFail(request, "fandemand out of range (0-200)");
        return;
      }
    }

    if (systemConfig.itho_control_interface == 1)
    {
      // RF CO2 mode: send auto + demand
      // Find first Send+RFTCO2 remote
      int rfIdx = -1;
      for (int ri = 0; ri < remotes.getMaxRemotes(); ri++)
      {
        if (remotes.isEmptySlot(ri)) continue;
        if (remotes.getRemoteFunction(ri) == RemoteFunctions::SEND &&
            remotes.getRemoteType(ri) == RemoteTypes::RFTCO2)
        {
          rfIdx = ri;
          break;
        }
      }
      if (rfIdx < 0)
      {
        sendFail(request, "no Send+RFTCO2 remote configured");
        return;
      }
      ithoExecRFCommand(rfIdx, "auto", HTMLAPI);
      delay(200);
      ithoSendRFDemand(rfIdx, (uint8_t)demand, 0, HTMLAPI);
      JsonDocument data;
      data["result"] = "auto + demand sent via RF CO2";
      data["demand"] = demand;
      data["index"] = rfIdx;
      if (!body["percentage"].isNull()) data["percentage"] = body["percentage"].as<int>();
      sendSuccess(request, data);
    }
    else if (systemConfig.itho_pwm2i2c == 1)
    {
      // PWM2I2C mode: map demand (0-200) to speed (0-255)
      int speed = (demand * 255) / 200;
      char speedStr[8];
      snprintf(speedStr, sizeof(speedStr), "%d", speed);
      ithoSetSpeed(speedStr, HTMLAPI);
      JsonDocument data;
      data["result"] = "speed set via PWM2I2C";
      data["speed"] = speed;
      data["demand"] = demand;
      if (!body["percentage"].isNull()) data["percentage"] = body["percentage"].as<int>();
      sendSuccess(request, data);
    }
    else
    {
      sendFail(request, "device does not support percentage/fandemand - no RF CO2 or PWM2I2C available");
    }
    return;
  }

  // Validate speed range before delegating
  if (!body["speed"].isNull())
  {
    int speed = body["speed"].as<int>();
    if (speed < 0 || speed > 255)
    {
      sendFail(request, "speed out of range (0-255)");
      return;
    }
  }
  if (!body["timer"].isNull())
  {
    int timer = body["timer"].as<int>();
    if (timer < 0 || timer > 65535)
    {
      sendFail(request, "timer out of range (0-65535)");
      return;
    }
  }

  // Convert JSON values to string params (process* functions expect const char*)
  JsonDocument params;
  if (!body["command"].isNull())
    params["command"] = body["command"];
  if (!body["speed"].isNull())
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", body["speed"].as<int>());
    params["speed"] = buf;
  }
  if (!body["timer"].isNull())
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", body["timer"].as<int>());
    params["timer"] = buf;
  }

  JsonDocument responseDoc;

  // Try named command first, then speed/timer
  auto status = processCommand(params.as<JsonObject>(), responseDoc);
  if (status == ApiResponse::status::CONTINUE)
    status = processPWMSpeedTimerCommands(params.as<JsonObject>(), responseDoc);

  if (status == ApiResponse::status::SUCCESS)
    sendSuccess(request, responseDoc);
  else if (status == ApiResponse::status::FAIL)
    sendFail(request, responseDoc["failreason"] | "command failed");
  else if (status == ApiResponse::status::ERROR)
    sendError(request, responseDoc["message"] | "internal error");
  else
    sendFail(request, "missing required field: command, speed, or timer");
}

// POST /api/v2/vremote
// Body: {"command":"low","index":0} or {"command":"low","name":"remote1"}
static void handlePostVRemote(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();

  // Map REST field names to the legacy parameter names
  JsonDocument params;
  if (!body["command"].isNull())
    params["vremotecmd"] = body["command"];
  if (!body["index"].isNull())
  {
    char idx[8];
    snprintf(idx, sizeof(idx), "%d", body["index"].as<int>());
    params["vremoteindex"] = idx;
  }
  if (!body["name"].isNull())
    params["vremotename"] = body["name"];

  JsonDocument responseDoc;
  auto status = processVremoteCommands(params.as<JsonObject>(), responseDoc);

  if (status == ApiResponse::status::SUCCESS)
    sendSuccess(request, responseDoc);
  else if (status == ApiResponse::status::FAIL)
    sendFail(request, responseDoc["failreason"] | "command failed");
  else
    sendFail(request, "missing required field: command");
}

// POST /api/v2/rfremote
// Body: {"command":"low","index":0}
static void handlePostRFRemote(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();
  JsonDocument params;
  if (!body["command"].isNull())
    params["rfremotecmd"] = body["command"];
  if (!body["index"].isNull())
  {
    char idx[8];
    snprintf(idx, sizeof(idx), "%d", body["index"].as<int>());
    params["rfremoteindex"] = idx;
  }

  JsonDocument responseDoc;
  auto status = processRFremoteCommands(params.as<JsonObject>(), responseDoc);

  if (status == ApiResponse::status::SUCCESS)
    sendSuccess(request, responseDoc);
  else if (status == ApiResponse::status::FAIL)
    sendFail(request, responseDoc["failreason"] | "command failed");
  else
    sendFail(request, "missing required field: command");
}

// POST /api/v2/rfco2
// Body: {"co2":800,"index":2}
static void handlePostRFCO2(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();
  if (body["co2"].isNull())
  {
    sendFail(request, "missing required field: co2");
    return;
  }

  uint16_t co2 = body["co2"].as<uint16_t>();
  uint8_t idx = body["index"] | 0;

  if (co2 > 10000)
  {
    sendFail(request, "co2 value out of range (0-10000)");
    return;
  }

  if (ithoSendRFCO2(idx, co2, HTMLAPI))
  {
    JsonDocument data;
    data["result"] = "rf co2 value sent";
    data["co2"] = co2;
    data["index"] = idx;
    sendSuccess(request, data);
  }
  else
  {
    sendFail(request, "rf co2 send failed - remote must be RFT CO2 type");
  }
}

// POST /api/v2/rfdemand
// Body: {"demand":150,"zone":0,"index":2}
static void handlePostRFDemand(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();
  if (body["demand"].isNull())
  {
    sendFail(request, "missing required field: demand");
    return;
  }

  uint8_t demand = body["demand"].as<uint8_t>();
  uint8_t zone = body["zone"] | 0;
  uint8_t idx = body["index"] | 0;

  if (body["demand"].as<int>() > 200)
  {
    sendFail(request, "demand value out of range (0-200)");
    return;
  }

  if (ithoSendRFDemand(idx, demand, zone, HTMLAPI))
  {
    JsonDocument data;
    data["result"] = "rf demand sent";
    data["demand"] = demand;
    data["zone"] = zone;
    data["index"] = idx;
    sendSuccess(request, data);
  }
  else
  {
    sendFail(request, "rf demand send failed - remote must be RFT CO2 or RFT RV type");
  }
}

// POST /api/v2/rf/config
// Body: {"index":1,"setting":"setrfdevicesourceid","value":"96,C8,B6"}
static void handlePostRFConfig(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();
  JsonDocument params;
  if (!body["index"].isNull())
  {
    char idx[8];
    snprintf(idx, sizeof(idx), "%d", body["index"].as<int>());
    params["setrfremote"] = idx;
  }
  if (!body["setting"].isNull())
    params["setting"] = body["setting"];
  if (!body["value"].isNull())
    params["settingvalue"] = body["value"];

  JsonDocument responseDoc;
  auto status = processSetRFremote(params.as<JsonObject>(), responseDoc);

  if (status == ApiResponse::status::SUCCESS)
    sendSuccess(request, responseDoc);
  else if (status == ApiResponse::status::FAIL)
    sendFail(request, responseDoc["failreason"] | "config update failed");
  else
    sendFail(request, "missing required fields: index, setting, value");
}

// PUT /api/v2/settings
// Body: {"index":5,"value":50}
static void handlePutSettings(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();
  if (body["index"].isNull() || body["value"].isNull())
  {
    sendFail(request, "missing required fields: index and value");
    return;
  }

  // Convert to the string-based params format expected by processSetsettingCommands
  JsonDocument params;
  char idxStr[12], valStr[20];
  snprintf(idxStr, sizeof(idxStr), "%d", body["index"].as<int>());
  // Handle both int and float values
  if (body["value"].is<float>() && body["value"].as<float>() != body["value"].as<int>())
    snprintf(valStr, sizeof(valStr), "%g", body["value"].as<double>());
  else
    snprintf(valStr, sizeof(valStr), "%d", body["value"].as<int>());
  params["setsetting"] = idxStr;
  params["value"] = valStr;

  JsonDocument responseDoc;
  auto status = processSetsettingCommands(params.as<JsonObject>(), responseDoc);

  if (status == ApiResponse::status::SUCCESS)
  {
    responseDoc["index"] = body["index"].as<int>();
    sendSuccess(request, responseDoc);
  }
  else if (status == ApiResponse::status::FAIL)
    sendFail(request, responseDoc["failreason"] | "setting update failed", responseDoc["code"] | 400);
  else if (status == ApiResponse::status::ERROR)
    sendError(request, responseDoc["message"] | "I2C error");
  else
    sendFail(request, "unable to process setting");
}

// POST /api/v2/debug
// Body: {"action":"reboot"} or {"action":"level1"}
static unsigned long lastRebootRequest = 0;

static void handlePostDebug(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();

  if (body["action"].isNull())
  {
    sendFail(request, "missing required field: action");
    return;
  }

  const char *action = body["action"];
  if (action != nullptr && strcmp(action, "reboot") == 0)
  {
    if (millis() - lastRebootRequest < 5000)
    {
      sendFail(request, "reboot rate limited - wait 5 seconds", 429);
      return;
    }
    lastRebootRequest = millis();
  }

  JsonDocument params;
  params["debug"] = body["action"];

  JsonDocument responseDoc;
  auto status = processDebugCommands(params.as<JsonObject>(), responseDoc);

  if (status == ApiResponse::status::SUCCESS)
    sendSuccess(request, responseDoc);
  else
    sendFail(request, responseDoc["failreason"] | "invalid action");
}

// POST /api/v2/outside_temp
// Body: {"temp":15.5}
static void handlePostOutsideTemp(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();
  if (body["temp"].isNull())
  {
    sendFail(request, "missing required field: temp");
    return;
  }

  JsonDocument params;
  char tempStr[12];
  snprintf(tempStr, sizeof(tempStr), "%d", body["temp"].as<int>());
  params["outside_temp"] = tempStr;

  JsonDocument responseDoc;
  auto status = processSetOutsideTemperature(params.as<JsonObject>(), responseDoc);

  if (status == ApiResponse::status::SUCCESS)
  {
    JsonDocument data;
    data["result"] = "outside temperature set";
    data["temp"] = body["temp"].as<int>();
    sendSuccess(request, data);
  }
  else if (status == ApiResponse::status::FAIL)
    sendFail(request, responseDoc["failreason"] | "temperature set failed");
  else
    sendError(request, responseDoc["message"] | "internal error");
}

// POST /api/v2/wpu/manual_control
// Body: {"index":1,"datatype":1,"value":1,"checked":1}
static void handlePostManualControl(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!checkRestAuth(request))
    return;

  JsonObject body = json.as<JsonObject>();
  if (body["index"].isNull())
  {
    sendFail(request, "missing required field: index");
    return;
  }

  uint16_t index = body["index"] | 0;
  uint8_t datatype = body["datatype"] | 0;
  uint16_t value = body["value"] | 0;
  uint8_t checked = body["checked"] | 0;

  setSetting4030(index, datatype, value, checked, false);

  JsonDocument data;
  data["result"] = "manual control command sent";
  data["index"] = index;
  data["datatype"] = datatype;
  data["value"] = value;
  data["checked"] = checked;
  sendSuccess(request, data);
}

// --- Route registration ---

// CORS preflight handler
static void handleOptions(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(204);
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  response->addHeader("Access-Control-Max-Age", "86400");
  request->send(response);
}

void registerRestAPIv2Routes(AsyncWebServer &server)
{
  // CORS preflight for all /api/v2/ endpoints
  server.on("/api/v2/speed", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/ithostatus", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/deviceinfo", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/queue", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/lastcmd", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/remotes", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/vremotes", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/rfstatus", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/command", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/vremote", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/rfremote/command", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/rfremote/co2", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/rfremote/demand", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/rfremote/config", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/debug", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/wpu/outside_temp", HTTP_OPTIONS, handleOptions);
  server.on("/api/v2/wpu/manual_control", HTTP_OPTIONS, handleOptions);

  // GET endpoints
  server.on("/api/v2/speed", HTTP_GET, handleGetSpeed);
  server.on("/api/v2/ithostatus", HTTP_GET, handleGetStatus);
  server.on("/api/v2/deviceinfo", HTTP_GET, handleGetDevice);
  server.on("/api/v2/ota", HTTP_GET, handleGetOTA);
  server.on("/api/v2/queue", HTTP_GET, handleGetQueue);
  server.on("/api/v2/lastcmd", HTTP_GET, handleGetLastCmd);
  server.on("/api/v2/remotes", HTTP_GET, handleGetRemotes);
  server.on("/api/v2/vremotes", HTTP_GET, handleGetVRemotes);
  server.on("/api/v2/rfstatus", HTTP_GET, handleGetRFStatus);
  server.on("/api/v2/settings", HTTP_GET, handleGetSettings);

  // POST/PUT endpoints use query parameters from the JSON body
  // parsed in the onBody handler (avoids AsyncCallbackJsonWebHandler dependency)
  server.on(
      "/api/v2/command", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostCommand(request, v);
        }
      });

  server.on(
      "/api/v2/vremote", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostVRemote(request, v);
        }
      });

  server.on(
      "/api/v2/rfremote/command", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostRFRemote(request, v);
        }
      });

  server.on(
      "/api/v2/rfremote/co2", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostRFCO2(request, v);
        }
      });

  server.on(
      "/api/v2/rfremote/demand", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostRFDemand(request, v);
        }
      });

  server.on(
      "/api/v2/rfremote/config", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostRFConfig(request, v);
        }
      });

  server.on(
      "/api/v2/ota", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostOTA(request, v);
        }
      });

  server.on(
      "/api/v2/debug", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostDebug(request, v);
        }
      });

  server.on(
      "/api/v2/wpu/outside_temp", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostOutsideTemp(request, v);
        }
      });

  server.on(
      "/api/v2/wpu/manual_control", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePostManualControl(request, v);
        }
      });

  // PUT endpoint
  server.on(
      "/api/v2/settings", HTTP_PUT,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (index + len == total)
        {
          JsonDocument json;
          if (deserializeJson(json, data, len))
          {
            sendFail(request, "invalid JSON body");
            return;
          }
          JsonVariant v = json.as<JsonVariant>();
          handlePutSettings(request, v);
        }
      });
}
