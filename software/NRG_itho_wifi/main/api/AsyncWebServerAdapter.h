// AsyncWebServerAdapter.h
#pragma once

#include <ESPAsyncWebServer.h>
#include "IWebServerHandler.h"

class AsyncWebServerAdapter : public IWebServerHandler
{
private:
    AsyncWebServerRequest *_request;

public:
    AsyncWebServerAdapter(AsyncWebServerRequest *request)
        : _request(request) {}

    void send(int code, const char *contentType, const std::string &content) override
    {
        AsyncWebServerResponse *response = _request->beginResponse(code, contentType, content.c_str());
        response->addHeader("Access-Control-Allow-Origin", "*");
        _request->send(response);
    }
};
