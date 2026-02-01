#pragma once

#include <ArduinoJson.h>
#include "api/ApiResponse.h"

class AsyncWebServerRequest;

void handleAPIv2(AsyncWebServerRequest *request);

ApiResponse::api_response_status_t checkAuthentication(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processGetCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processGetsettingCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processSetsettingCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processCommand(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processSetRFremote(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processRFremoteCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processVremoteCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processPWMSpeedTimerCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processi2csnifferCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processDebugCommands(JsonObject params, JsonDocument &response);
ApiResponse::api_response_status_t processSetOutsideTemperature(JsonObject params, JsonDocument &response);
