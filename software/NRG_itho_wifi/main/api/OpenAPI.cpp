#include "api/OpenAPI.h"
#include "config/SystemConfig.h"
#include "globals.h"
#include <ArduinoJson.h>

// Helper to add a query parameter to the parameters array
static JsonObject addParam(JsonArray &params, const char *name, const char *description,
                           const char *type, int minimum = -1, int maximum = -1)
{
  JsonObject p = params.add<JsonObject>();
  p["name"] = name;
  p["in"] = "query";
  p["description"] = description;
  JsonObject schema = p["schema"].to<JsonObject>();
  schema["type"] = type;
  if (minimum >= 0)
    schema["minimum"] = minimum;
  if (maximum >= 0)
    schema["maximum"] = maximum;
  return p;
}

// Helper to add a string enum parameter
static void addEnumParam(JsonArray &params, const char *name, const char *description,
                         std::initializer_list<const char *> values)
{
  JsonObject p = addParam(params, name, description, "string");
  JsonArray enumArr = p["schema"]["enum"].to<JsonArray>();
  for (auto v : values)
    enumArr.add(v);
}

// Helper to add a property to a schema
static void addProp(JsonObject &props, const char *name, const char *type,
                    const char *description = nullptr, int minimum = -1, int maximum = -1)
{
  JsonObject p = props[name].to<JsonObject>();
  p["type"] = type;
  if (description)
    p["description"] = description;
  if (minimum >= 0)
    p["minimum"] = minimum;
  if (maximum >= 0)
    p["maximum"] = maximum;
}

void handleOpenAPI(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_api)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }

  JsonDocument doc;

  // Root
  doc["openapi"] = "3.0.3";

  // Info
  JsonObject info = doc["info"].to<JsonObject>();
  info["title"] = "IthoWifi API";
  info["version"] = fw_version;
  info["description"] = "API for controlling Itho ventilation units via the IthoWifi add-on. "
                         "Supports both URL query parameters (WebAPI) and MQTT JSON commands.";
  JsonObject license = info["license"].to<JsonObject>();
  license["name"] = "GPL-3.0";
  license["url"] = "https://github.com/arjenhiemstra/ithowifi/blob/master/LICENSE";

  // Servers
  JsonObject server = doc["servers"].add<JsonObject>();
  server["url"] = "/";
  server["description"] = "This device";

  // === Paths ===

  // GET /api.html
  JsonObject apiGet = doc["paths"]["/api.html"]["get"].to<JsonObject>();
  apiGet["summary"] = "Execute API command";
  apiGet["description"] = "All commands are passed as URL query parameters. Response follows the JSend specification.";
  apiGet["tags"].add("WebAPI");

  JsonArray params = apiGet["parameters"].to<JsonArray>();

  // General commands
  addEnumParam(params, "command", "Fan speed/timer command",
               {"low", "medium", "high", "timer1", "timer2", "timer3", "away", "cook30", "cook60", "autonight", "clearqueue"});
  addParam(params, "speed", "Set fan speed directly (0-255)", "integer", 0, 255);
  addParam(params, "timer", "Timer in minutes (use with speed or command)", "integer", 0, 65535);

  // Virtual remote
  addEnumParam(params, "vremotecmd", "Virtual remote command (I2C)",
               {"away", "low", "medium", "high", "timer1", "timer2", "timer3", "join", "leave", "auto", "autonight", "cook30", "cook60"});
  addParam(params, "vremoteindex", "Virtual remote index (default 0)", "integer", 0, 11);
  addParam(params, "vremotename", "Select virtual remote by name", "string");

  // Get commands
  addEnumParam(params, "get", "Retrieve data",
               {"ithostatus", "remotesinfo", "vremotesinfo", "deviceinfo", "currentspeed", "rfstatus"});
  addParam(params, "name", "Filter name for get=rfstatus", "string");

  // Settings
  addParam(params, "getsetting", "Get Itho setting by index", "integer", 0, 255);
  addParam(params, "setsetting", "Set Itho setting by index (use with value)", "integer", 0, 255);
  addParam(params, "value", "Value for setsetting", "number");

  // RF remote (CC1101)
  addEnumParam(params, "rfremotecmd", "RF remote command via CC1101",
               {"away", "low", "medium", "high", "timer1", "timer2", "timer3", "join", "leave", "auto", "autonight", "cook30", "cook60", "motion_on", "motion_off"});
  addParam(params, "rfremoteindex", "RF remote index (default 0)", "integer", 0, 11);
  addParam(params, "rfco2", "Send CO2 ppm via RF (requires RFT CO2)", "integer", 0, 10000);
  addParam(params, "rfdemand", "Send ventilation demand via RF (0-200, requires RFT CO2/RV)", "integer", 0, 200);
  addParam(params, "rfzone", "Zone for rfdemand (default 0)", "integer", 0, 255);

  // WPU
  addParam(params, "outside_temp", "Set outside temperature (WPU only)", "number");

  // Response
  JsonObject resp200 = apiGet["responses"]["200"].to<JsonObject>();
  resp200["description"] = "JSend response";
  resp200["content"]["application/json"]["schema"]["$ref"] = "#/components/schemas/ApiResponse";

  // Mark legacy API as deprecated
  apiGet["deprecated"] = true;

  // === REST API v2 endpoints ===

  auto addRestGet = [&](const char *path, const char *summary)
  {
    JsonObject op = doc["paths"][path]["get"].to<JsonObject>();
    op["summary"] = summary;
    op["tags"].add("REST API v2");
    op["responses"]["200"]["description"] = "JSend success";
    op["responses"]["200"]["content"]["application/json"]["schema"]["$ref"] = "#/components/schemas/ApiResponse";
    return op;
  };

  auto addRestPost = [&](const char *path, const char *summary, const char *schemaRef)
  {
    JsonObject op = doc["paths"][path]["post"].to<JsonObject>();
    op["summary"] = summary;
    op["tags"].add("REST API v2");
    op["requestBody"]["required"] = true;
    op["requestBody"]["content"]["application/json"]["schema"]["$ref"] = schemaRef;
    op["responses"]["200"]["description"] = "JSend success";
    op["responses"]["200"]["content"]["application/json"]["schema"]["$ref"] = "#/components/schemas/ApiResponse";
    op["responses"]["400"]["description"] = "JSend fail (validation error)";
    return op;
  };

  // GET endpoints
  addRestGet("/api/v2/speed", "Get current fan speed");
  addRestGet("/api/v2/status", "Get Itho device status");
  addRestGet("/api/v2/device", "Get device info");
  addRestGet("/api/v2/queue", "Get command queue");
  addRestGet("/api/v2/lastcmd", "Get last executed command");
  addRestGet("/api/v2/remotes", "Get RF remotes configuration");
  addRestGet("/api/v2/vremotes", "Get virtual remotes info");

  JsonObject rfstatusGet = addRestGet("/api/v2/rfstatus", "Get RF device status");
  JsonArray rfstatusParams = rfstatusGet["parameters"].to<JsonArray>();
  addParam(rfstatusParams, "name", "Filter by source name", "string");

  JsonObject settingsGet = addRestGet("/api/v2/settings", "Read Itho setting by index");
  JsonArray settingsParams = settingsGet["parameters"].to<JsonArray>();
  addParam(settingsParams, "index", "Setting index", "integer", 0, 255);

  // POST endpoints
  addRestPost("/api/v2/command", "Send fan command", "#/components/schemas/CommandRequest");
  addRestPost("/api/v2/vremote", "Send virtual remote command", "#/components/schemas/VRemoteRequest");
  addRestPost("/api/v2/rfremote", "Send RF remote command", "#/components/schemas/RFRemoteRequest");
  addRestPost("/api/v2/rfco2", "Send CO2 ppm via RF", "#/components/schemas/RFCO2Request");
  addRestPost("/api/v2/rfdemand", "Send ventilation demand via RF", "#/components/schemas/RFDemandRequest");
  addRestPost("/api/v2/rf/config", "Configure RF remote", "#/components/schemas/RFConfigRequest");
  addRestPost("/api/v2/debug", "Debug actions (reboot, RF debug level)", "#/components/schemas/DebugRequest");
  addRestPost("/api/v2/outside_temp", "Set outside temperature", "#/components/schemas/OutsideTempRequest");

  // PUT endpoint
  JsonObject settingsPut = doc["paths"]["/api/v2/settings"]["put"].to<JsonObject>();
  settingsPut["summary"] = "Write Itho setting";
  settingsPut["tags"].add("REST API v2");
  settingsPut["requestBody"]["required"] = true;
  settingsPut["requestBody"]["content"]["application/json"]["schema"]["$ref"] = "#/components/schemas/SetSettingRequest";
  settingsPut["responses"]["200"]["description"] = "JSend success";
  settingsPut["responses"]["400"]["description"] = "JSend fail";
  settingsPut["responses"]["403"]["description"] = "Settings API disabled";

  // Documentation endpoints
  JsonObject specPath = doc["paths"]["/api/openapi.json"]["get"].to<JsonObject>();
  specPath["summary"] = "OpenAPI specification";
  specPath["tags"].add("Documentation");
  specPath["responses"]["200"]["description"] = "OpenAPI 3.0 JSON spec";

  // === Components ===

  // ApiResponse schema
  JsonObject apiRespProps = doc["components"]["schemas"]["ApiResponse"]["properties"].to<JsonObject>();
  doc["components"]["schemas"]["ApiResponse"]["type"] = "object";
  addProp(apiRespProps, "status", "string");
  apiRespProps["status"]["enum"].add("success");
  apiRespProps["status"]["enum"].add("fail");
  apiRespProps["status"]["enum"].add("error");
  addProp(apiRespProps, "data", "object", "Response data");
  addProp(apiRespProps, "message", "string", "Error message");
  addProp(apiRespProps, "timestamp", "integer", "Unix timestamp");

  // MqttCommand schema
  JsonObject mqttSchema = doc["components"]["schemas"]["MqttCommand"].to<JsonObject>();
  mqttSchema["type"] = "object";
  mqttSchema["description"] = "MQTT JSON command (send to command topic)";
  JsonObject mqttProps = mqttSchema["properties"].to<JsonObject>();
  addProp(mqttProps, "command", "string");
  mqttProps["command"]["enum"].add("low");
  mqttProps["command"]["enum"].add("medium");
  mqttProps["command"]["enum"].add("high");
  mqttProps["command"]["enum"].add("timer1");
  mqttProps["command"]["enum"].add("timer2");
  mqttProps["command"]["enum"].add("timer3");
  mqttProps["command"]["enum"].add("away");
  mqttProps["command"]["enum"].add("cook30");
  mqttProps["command"]["enum"].add("cook60");
  mqttProps["command"]["enum"].add("autonight");
  mqttProps["command"]["enum"].add("clearqueue");
  addProp(mqttProps, "speed", "integer", nullptr, 0, 255);
  addProp(mqttProps, "timer", "integer", nullptr, 0, 65535);
  addProp(mqttProps, "vremotecmd", "string");
  addProp(mqttProps, "vremoteindex", "integer");
  addProp(mqttProps, "vremotename", "string");
  addProp(mqttProps, "rfremotecmd", "string");
  addProp(mqttProps, "rfremoteindex", "integer");
  addProp(mqttProps, "rfco2", "integer", "CO2 ppm");
  addProp(mqttProps, "rfdemand", "integer", "Ventilation demand 0-200");
  addProp(mqttProps, "rfzone", "integer", "Zone for rfdemand");
  addProp(mqttProps, "dtype", "string", "Domoticz device type");

  // === REST API v2 request body schemas ===

  // CommandRequest
  auto addSchema = [&](const char *name, const char *desc) -> JsonObject
  {
    JsonObject s = doc["components"]["schemas"][name].to<JsonObject>();
    s["type"] = "object";
    if (desc)
      s["description"] = desc;
    return s["properties"].to<JsonObject>();
  };

  JsonObject cmdProps = addSchema("CommandRequest", "Fan command or speed/timer");
  addProp(cmdProps, "command", "string", "Named command");
  for (auto cmd : {"low", "medium", "high", "timer1", "timer2", "timer3", "away", "cook30", "cook60", "autonight", "clearqueue"})
    cmdProps["command"]["enum"].add(cmd);
  addProp(cmdProps, "speed", "integer", "Fan speed", 0, 255);
  addProp(cmdProps, "timer", "integer", "Timer in minutes", 0, 65535);

  JsonObject vrProps = addSchema("VRemoteRequest", "Virtual remote command");
  addProp(vrProps, "command", "string", "Command name");
  for (auto cmd : {"away", "low", "medium", "high", "timer1", "timer2", "timer3", "join", "leave", "auto", "autonight", "cook30", "cook60"})
    vrProps["command"]["enum"].add(cmd);
  addProp(vrProps, "index", "integer", "Virtual remote index", 0, 11);
  addProp(vrProps, "name", "string", "Virtual remote name (alternative to index)");

  JsonObject rfProps = addSchema("RFRemoteRequest", "RF remote command");
  addProp(rfProps, "command", "string", "RF command name");
  for (auto cmd : {"away", "low", "medium", "high", "timer1", "timer2", "timer3", "join", "leave", "auto", "autonight", "cook30", "cook60", "motion_on", "motion_off"})
    rfProps["command"]["enum"].add(cmd);
  addProp(rfProps, "index", "integer", "RF remote index", 0, 11);

  JsonObject co2Props = addSchema("RFCO2Request", "Send CO2 value via RF");
  addProp(co2Props, "co2", "integer", "CO2 level in ppm", 0, 10000);
  addProp(co2Props, "index", "integer", "RF remote index (default 0)", 0, 11);

  JsonObject demandProps = addSchema("RFDemandRequest", "Send ventilation demand via RF");
  addProp(demandProps, "demand", "integer", "Demand level (0=0%, 200=100%)", 0, 200);
  addProp(demandProps, "zone", "integer", "Zone (default 0)", 0, 255);
  addProp(demandProps, "index", "integer", "RF remote index (default 0)", 0, 11);

  JsonObject rfcfgProps = addSchema("RFConfigRequest", "Configure RF remote settings");
  addProp(rfcfgProps, "index", "integer", "RF remote index", 0, 11);
  addProp(rfcfgProps, "setting", "string", "Setting name (setrfdevicesourceid/setrfdevicedestid/setrfdevicebidirectional)");
  addProp(rfcfgProps, "value", "string", "Setting value (hex ID like 96,C8,B6 or true/false)");

  JsonObject debugProps = addSchema("DebugRequest", "Debug/reboot actions");
  addProp(debugProps, "action", "string", "Action (reboot/level0/level1/level2/level3)");

  JsonObject tempProps = addSchema("OutsideTempRequest", "Set outside temperature");
  addProp(tempProps, "temp", "number", "Temperature in Celsius (-100 to 100)");

  JsonObject setProps = addSchema("SetSettingRequest", "Write Itho device setting");
  addProp(setProps, "index", "integer", "Setting index", 0, 255);
  addProp(setProps, "value", "number", "New value (int or float, must be within setting min/max)");

  // Serialize and send
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Access-Control-Allow-Origin", "*");
  serializeJson(doc, *response);
  request->send(response);
}
