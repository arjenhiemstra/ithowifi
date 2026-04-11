#include <string>

#include <Arduino.h>
#include "globals.h"

#include "config/HADiscConfig.h"
#include "cc1101/IthoPacket.h"

HADiscConfig haDiscConfig;

// RF sensor availability per remote type
// Format: comma-separated list of available sensors
const char* HADiscConfig::getAvailableSensorsForType(uint16_t remoteType)
{
    switch (remoteType)
    {
    case RFTCO2:
        return "lastcmd,temp,co2";
    case RFTRV:
        return "lastcmd,temp,hum,dewpoint,battery";
    case RFTPIR:
        return "lastcmd,pir";
    case RFTCVE:
    case RFTAUTO:
    case RFTN:
    case RFTAUTON:
    case DEMANDFLOW:
    case RFTSPIDER:
    case ORCON15LF01:
    default:
        return "lastcmd";
    }
}

bool HADiscConfig::isSensorAvailableForType(uint16_t remoteType, const char* sensor)
{
    const char* available = getAvailableSensorsForType(remoteType);
    return strstr(available, sensor) != nullptr;
}

// RF fan presets per remote type.
// Derived from the RF remote command maps in IthoCC1101.cpp (RFT*_Remote_Map):
// a preset is listed here only if the remote type actually has a non-null
// command entry for it. Kept in lockstep with getWirePresetsForType() below.
const char* HADiscConfig::getPresetsForType(uint16_t remoteType)
{
    switch (remoteType)
    {
    case RFTCVE:
        return "Away,Low,Medium,High,Timer 10min,Timer 20min,Timer 30min";
    case RFTAUTO:
        return "Low,Auto,High,AutoNight,Timer 10min,Timer 20min,Timer 30min";
    case RFTN:
        return "Away,Low,Medium,High,Timer 10min,Timer 20min,Timer 30min";
    case RFTAUTON:
        return "Away,Low,Medium,High,Auto,AutoNight,Timer 10min,Timer 20min,Timer 30min";
    case DEMANDFLOW:
        return "Low,High,Timer 10min,Timer 20min,Timer 30min,Cook 30min,Cook 60min";
    case RFTRV:
    case RFTCO2:
        return "Low,Medium,High,Auto,AutoNight,Timer 10min,Timer 20min,Timer 30min";
    case RFTSPIDER:
        return "Low,Medium,High,Auto,AutoNight,Timer 10min,Timer 20min,Timer 30min";
    case ORCON15LF01:
        return "Away,Auto,Low,Medium,High,Timer 10min,Timer 20min,Timer 30min";
    case RFTPIR:
        return ""; // PIR doesn't have fan presets, it's motion-triggered
    default:
        return "Low,Medium,High";
    }
}

// Wire-format presets per remote type. Same set as getPresetsForType(), but
// using the short API command names so external consumers (HA integration,
// scripts) can dispatch without a translation table. Kept in lockstep with
// getPresetsForType() — if you add a preset to one, add it to the other.
const char* HADiscConfig::getWirePresetsForType(uint16_t remoteType)
{
    switch (remoteType)
    {
    case RFTCVE:
        return "away,low,medium,high,timer1,timer2,timer3";
    case RFTAUTO:
        return "low,auto,high,autonight,timer1,timer2,timer3";
    case RFTN:
        return "away,low,medium,high,timer1,timer2,timer3";
    case RFTAUTON:
        return "away,low,medium,high,auto,autonight,timer1,timer2,timer3";
    case DEMANDFLOW:
        return "low,high,timer1,timer2,timer3,cook30,cook60";
    case RFTRV:
    case RFTCO2:
        return "low,medium,high,auto,autonight,timer1,timer2,timer3";
    case RFTSPIDER:
        return "low,medium,high,auto,autonight,timer1,timer2,timer3";
    case ORCON15LF01:
        return "away,auto,low,medium,high,timer1,timer2,timer3";
    case RFTPIR:
        return "";
    default:
        return "low,medium,high";
    }
}

// default constructor
HADiscConfig::HADiscConfig()
{
    // default config
    reset();

    configLoaded = false;

} // HADiscConfig

// default destructor
HADiscConfig::~HADiscConfig()
{
} //~HADiscConfig

bool HADiscConfig::set(JsonObject obj)
{
    bool updated = false;

    if (!obj["sscnt"].isNull())
    {
        updated = true;
        sscnt = obj["sscnt"];
    }
    if (!obj["d"].isNull())
    {
        updated = true;
        strlcpy(d, obj["d"] | "", sizeof(d));
    }
    if (!obj["c"].isNull())
    {
        updated = true;
        copyArray(obj["c"].as<JsonArray>(), itemsDoc);
    }
    if (!obj["rf"].isNull())
    {
        updated = true;
        copyArray(obj["rf"].as<JsonArray>(), rfDoc);
    }
    if (!obj["vr"].isNull())
    {
        updated = true;
        copyArray(obj["vr"].as<JsonArray>(), vrDoc);
    }

    return updated;
}

void HADiscConfig::get(JsonObject obj) const
{
    obj["sscnt"] = sscnt;
    obj["d"] = d;
    obj["c"].set(itemsDoc.as<JsonArrayConst>());
    obj["rf"].set(rfDoc.as<JsonArrayConst>());
    obj["vr"].set(vrDoc.as<JsonArrayConst>());
}

void HADiscConfig::reset()
{
    sscnt = 0;                                                             // saved_status_count
    strlcpy(d, "unset", sizeof(d));                                        // device_name
    JsonArray itemsArr __attribute__((unused)) = itemsDoc.to<JsonArray>(); // components
    JsonArray rfArr __attribute__((unused)) = rfDoc.to<JsonArray>();       // RF device sensors
    JsonArray vrArr __attribute__((unused)) = vrDoc.to<JsonArray>();       // Virtual remote fans
}
