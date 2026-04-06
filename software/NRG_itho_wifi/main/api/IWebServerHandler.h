// IWebServerHandler.h
#pragma once

#include <Arduino.h>

class IWebServerHandler
{
public:
    virtual void send(int code, const char *contentType, const std::string &content) = 0;
    virtual ~IWebServerHandler() {}
};


