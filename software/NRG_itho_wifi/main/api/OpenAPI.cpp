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

  // GET /api/openapi.json
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

  // Serialize and send
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Access-Control-Allow-Origin", "*");
  serializeJson(doc, *response);
  request->send(response);
}
