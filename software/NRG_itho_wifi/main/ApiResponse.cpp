// ApiResponse.cpp
#include "ApiResponse.h"

ApiResponse::ApiResponse(IWebServerHandler *webHandler, PubSubClient *mqttClient)
    : _webHandler(webHandler), _mqttClient(mqttClient) {}

void ApiResponse::sendSuccess(const char *message, const char *output, const char *mqttTopic)
{
    JsonDocument doc;
    doc["status"] = "success";
    JsonObject data = doc["data"].to<JsonObject>();
    data["message"] = message;
    send(doc, 200, output);
}

void ApiResponse::sendSuccess(const JsonDocument &jsonMessage, const char *output, const char *mqttTopic)
{
    JsonDocument doc;
    doc["status"] = "success";
    doc["data"] = jsonMessage;
    send(doc, 200, output);
}

void ApiResponse::sendFail(const char *message, const char *output, const char *mqttTopic)
{
    JsonDocument doc;
    doc["status"] = "fail";
    doc["data"] = message;
    send(doc, 400, output);
}

void ApiResponse::sendFail(const JsonDocument &jsonMessage, const char *output, const char *mqttTopic)
{
    JsonDocument doc;
    doc["status"] = "fail";
    doc["data"] = jsonMessage;
    send(doc, 400, output);
}

void ApiResponse::sendError(const char *message, const char *output, const char *mqttTopic)
{
    JsonDocument doc;
    doc["status"] = "error";
    doc["message"] = message;
    send(doc, 500, output);
}

void ApiResponse::sendError(const JsonDocument &jsonMessage, const char *output, const char *mqttTopic)
{
    JsonDocument doc;
    doc["status"] = "error";
    doc["message"] = jsonMessage;
    send(doc, 500, output);
}

void ApiResponse::send(const JsonDocument &jsonMessage, int statusCode, const char *output, const char *mqttTopic)
{
    std::string response;
    serializeJson(jsonMessage, response);
    if (strcmp(output, "http") == 0 && _webHandler != nullptr)
    {
        _webHandler->send(statusCode, "application/json", response);
    }
    else if (strcmp(output, "mqtt") == 0 && _mqttClient != nullptr && mqttTopic != nullptr && _mqttClient->connected())
    {
        _mqttClient->publish(mqttTopic, response.c_str());
    }
}
