#include <string>

#include <Arduino.h>
#include "globals.h"

#include "config/HADiscConfig.h"

HADiscConfig haDiscConfig;

// default constructor
HADiscConfig::HADiscConfig()
{
    // default config

    sscnt = 0;                                                             // saved_status_count
    strlcpy(d, "unset", sizeof(d));                                        // device_name
    JsonArray itemsArr __attribute__((unused)) = itemsDoc.to<JsonArray>(); // components

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

    return updated;
}

void HADiscConfig::get(JsonObject obj) const
{
    obj["sscnt"] = sscnt;
    obj["d"] = d;
    obj["c"].set(itemsDoc.as<JsonArrayConst>());
}
