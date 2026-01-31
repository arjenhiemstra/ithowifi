// ApiResponse.h
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "IWebServerHandler.h"

class ApiResponse
{
private:
    IWebServerHandler *_webHandler;
    PubSubClient *_mqttClient;

    void send(const JsonDocument &jsonMessage, int statusCode, const char *output, const char *mqttTopic = nullptr);

public:
    enum status
    {
        CONTINUE,
        SUCCESS,
        FAIL,
        ERROR
    };
    typedef status api_response_status_t;

    ApiResponse(IWebServerHandler *webHandler, PubSubClient *mqttClient);
    void sendSuccess(const char *message, const char *output = "http", const char *mqttTopic = nullptr);
    void sendSuccess(const JsonDocument &jsonMessage, const char *output = "http", const char *mqttTopic = nullptr);
    void sendFail(const char *message, const char *output = "http", const char *mqttTopic = nullptr);
    void sendFail(const JsonDocument &jsonMessage, const char *output = "http", const char *mqttTopic = nullptr);
    void sendError(const char *message, const char *output = "http", const char *mqttTopic = nullptr);
    void sendError(const JsonDocument &jsonMessage, const char *output = "http", const char *mqttTopic = nullptr);
};
