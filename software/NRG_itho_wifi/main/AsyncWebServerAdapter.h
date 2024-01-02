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

    void send(int code, const char *contentType, const String &content) override
    {
        _request->send(code, contentType, content);
    }
};
