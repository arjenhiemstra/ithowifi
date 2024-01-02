// IWebServerHandler.h
#pragma once

#include <Arduino.h>

class IWebServerHandler
{
public:
    virtual void send(int code, const char *contentType, const String &content) = 0;
    virtual ~IWebServerHandler() {}
};


