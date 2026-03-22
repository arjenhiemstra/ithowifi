#include "api/OpenAPI.h"
#include "config/SystemConfig.h"
#include "globals.h"

#define J(s) R"J(" s ")J"

void handleOpenAPI(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_api)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }

  char version[16]{};
  strlcpy(version, fw_version, sizeof(version));

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->addHeader("Access-Control-Allow-Origin", "*");

  // Info
  response->printf("{\"openapi\":\"3.0.3\",\"info\":{\"title\":\"IthoWifi API\",\"version\":\"%s\",", version);
  response->print("\"description\":\"API for controlling Itho ventilation units via the IthoWifi add-on. Supports both URL query parameters (WebAPI) and MQTT JSON commands.\",");
  response->print("\"license\":{\"name\":\"GPL-3.0\",\"url\":\"https://github.com/arjenhiemstra/ithowifi/blob/master/LICENSE\"}},");
  response->print("\"servers\":[{\"url\":\"/\",\"description\":\"This device\"}],");

  // Paths
  response->print("\"paths\":{\"/api.html\":{\"get\":{");
  response->print("\"summary\":\"Execute API command\",\"description\":\"All commands are passed as URL query parameters. Response follows the JSend specification.\",");
  response->print("\"tags\":[\"WebAPI\"],\"parameters\":[");

  // General commands
  response->print("{\"name\":\"command\",\"in\":\"query\",\"description\":\"Fan speed/timer command\",\"schema\":{\"type\":\"string\",\"enum\":[\"low\",\"medium\",\"high\",\"timer1\",\"timer2\",\"timer3\",\"away\",\"cook30\",\"cook60\",\"autonight\",\"clearqueue\"]}},");
  response->print("{\"name\":\"speed\",\"in\":\"query\",\"description\":\"Set fan speed directly (0-255)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255}},");
  response->print("{\"name\":\"timer\",\"in\":\"query\",\"description\":\"Timer in minutes (use with speed or command)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":65535}},");

  // Virtual remote
  response->print("{\"name\":\"vremotecmd\",\"in\":\"query\",\"description\":\"Virtual remote command (I2C)\",\"schema\":{\"type\":\"string\",\"enum\":[\"away\",\"low\",\"medium\",\"high\",\"timer1\",\"timer2\",\"timer3\",\"join\",\"leave\",\"auto\",\"autonight\",\"cook30\",\"cook60\"]}},");
  response->print("{\"name\":\"vremoteindex\",\"in\":\"query\",\"description\":\"Virtual remote index (default 0)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":11}},");
  response->print("{\"name\":\"vremotename\",\"in\":\"query\",\"description\":\"Select virtual remote by name\",\"schema\":{\"type\":\"string\"}},");

  // Get commands
  response->print("{\"name\":\"get\",\"in\":\"query\",\"description\":\"Retrieve data\",\"schema\":{\"type\":\"string\",\"enum\":[\"ithostatus\",\"remotesinfo\",\"vremotesinfo\",\"deviceinfo\",\"currentspeed\",\"rfstatus\"]}},");
  response->print("{\"name\":\"name\",\"in\":\"query\",\"description\":\"Filter name for get=rfstatus\",\"schema\":{\"type\":\"string\"}},");

  // Settings
  response->print("{\"name\":\"getsetting\",\"in\":\"query\",\"description\":\"Get Itho setting by index\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255}},");
  response->print("{\"name\":\"setsetting\",\"in\":\"query\",\"description\":\"Set Itho setting by index (use with value)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255}},");
  response->print("{\"name\":\"value\",\"in\":\"query\",\"description\":\"Value for setsetting\",\"schema\":{\"type\":\"number\"}},");

  // RF remote (CC1101)
  response->print("{\"name\":\"rfremotecmd\",\"in\":\"query\",\"description\":\"RF remote command via CC1101\",\"schema\":{\"type\":\"string\",\"enum\":[\"away\",\"low\",\"medium\",\"high\",\"timer1\",\"timer2\",\"timer3\",\"join\",\"leave\",\"auto\",\"autonight\",\"cook30\",\"cook60\",\"motion_on\",\"motion_off\"]}},");
  response->print("{\"name\":\"rfremoteindex\",\"in\":\"query\",\"description\":\"RF remote index (default 0)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":11}},");
  response->print("{\"name\":\"rfco2\",\"in\":\"query\",\"description\":\"Send CO2 ppm via RF (requires RFT CO2)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":10000}},");
  response->print("{\"name\":\"rfdemand\",\"in\":\"query\",\"description\":\"Send ventilation demand via RF (0-200, requires RFT CO2/RV)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":200}},");
  response->print("{\"name\":\"rfzone\",\"in\":\"query\",\"description\":\"Zone for rfdemand (default 0)\",\"schema\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255}},");

  // WPU
  response->print("{\"name\":\"outside_temp\",\"in\":\"query\",\"description\":\"Set outside temperature (WPU only)\",\"schema\":{\"type\":\"number\"}}");

  // End parameters, close responses + get
  response->print("],\"responses\":{\"200\":{\"description\":\"JSend response\",\"content\":{\"application/json\":{\"schema\":{\"$ref\":\"#/components/schemas/ApiResponse\"}}}}}}},");

  // Other paths
  response->print("\"/api/openapi.json\":{\"get\":{\"summary\":\"OpenAPI specification\",\"tags\":[\"Documentation\"],\"responses\":{\"200\":{\"description\":\"OpenAPI 3.0 JSON spec\"}}}}");

  response->print("},"); // close paths

  // Components
  response->print("\"components\":{\"schemas\":{");

  // ApiResponse
  response->print("\"ApiResponse\":{\"type\":\"object\",\"properties\":{");
  response->print("\"status\":{\"type\":\"string\",\"enum\":[\"success\",\"fail\",\"error\"]},");
  response->print("\"data\":{\"type\":\"object\",\"description\":\"Response data\"},");
  response->print("\"message\":{\"type\":\"string\",\"description\":\"Error message\"},");
  response->print("\"timestamp\":{\"type\":\"integer\",\"description\":\"Unix timestamp\"}}},");

  // MqttCommand
  response->print("\"MqttCommand\":{\"type\":\"object\",\"description\":\"MQTT JSON command (send to command topic)\",\"properties\":{");
  response->print("\"command\":{\"type\":\"string\",\"enum\":[\"low\",\"medium\",\"high\",\"timer1\",\"timer2\",\"timer3\",\"away\",\"cook30\",\"cook60\",\"autonight\",\"clearqueue\"]},");
  response->print("\"speed\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255},");
  response->print("\"timer\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":65535},");
  response->print("\"vremotecmd\":{\"type\":\"string\"},");
  response->print("\"vremoteindex\":{\"type\":\"integer\"},");
  response->print("\"vremotename\":{\"type\":\"string\"},");
  response->print("\"rfremotecmd\":{\"type\":\"string\"},");
  response->print("\"rfremoteindex\":{\"type\":\"integer\"},");
  response->print("\"rfco2\":{\"type\":\"integer\",\"description\":\"CO2 ppm\"},");
  response->print("\"rfdemand\":{\"type\":\"integer\",\"description\":\"Ventilation demand 0-200\"},");
  response->print("\"rfzone\":{\"type\":\"integer\",\"description\":\"Zone for rfdemand\"},");
  response->print("\"dtype\":{\"type\":\"string\",\"description\":\"Domoticz device type\"}");
  response->print("}}"); // close MqttCommand

  response->print("}}}"); // close schemas, components, root

  request->send(response);
}

