#include <string>

#include <Arduino.h>
#include "globals.h"

#include "LogConfig.h"

LogConfig logConfig;

// default constructor
LogConfig::LogConfig()
{
    // default config

    loglevel = 5; // 5=NOTICE
    syslog_active = 0;
    strlcpy(logserver, "192.168.0.2", sizeof(logserver));
    logport = 514;
    strlcpy(logref, "nrgitho", sizeof(logref));
    strlcpy(config_struct_version, LOG_CONFIG_VERSION, sizeof(config_struct_version));

    configLoaded = false;

} // LogConfig

// default destructor
LogConfig::~LogConfig()
{
} //~LogConfig

bool LogConfig::set(JsonObjectConst obj)
{
    bool updated = false;

    if (!(const char *)obj["loglevel"].isNull())
    {
        updated = true;
        loglevel = obj["loglevel"];
    }
    if (!(const char *)obj["syslog_active"].isNull())
    {
        updated = true;
        syslog_active = obj["syslog_active"];
    }
    if (!(const char *)obj["logserver"].isNull())
    {
        updated = true;
        strlcpy(logserver, obj["logserver"], sizeof(logserver));
    }
    if (!(const char *)obj["logport"].isNull())
    {
        updated = true;
        logport = obj["logport"];
    }
    if (!(const char *)obj["logref"].isNull())
    {
        updated = true;
        strlcpy(logref, obj["logref"], sizeof(logref));
    }
    return updated;
}

void LogConfig::get(JsonObject obj) const
{
    obj["loglevel"] = loglevel;
    obj["syslog_active"] = syslog_active;
    obj["logserver"] = logserver;
    obj["logport"] = logport;
    obj["logref"] = logref;
}
