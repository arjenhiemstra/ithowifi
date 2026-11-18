#pragma once
// RF kWh meters heard by the CC1101 (Itho/Klimaatgarant, RAMSES type 29): store, RF callback and JSON.
// Readers take rfStatusMutex, the RF task writes under it.

#include <ArduinoJson.h>
#include "cc1101/rf_kwh_meter.h"

extern rfkwh::Store rfKwhMeters;

// Callback for IthoCC1101::setRFKwhCallback: decodes RP 2401 and updates the store
void parseRFKwhMeter(const uint8_t *payload, uint8_t len, uint8_t srcId0, uint8_t srcId1, uint8_t srcId2);

// One meter: id, e1_kwh..e3_kwh, lastseen, frames
void getRFKwhMeterJSON(const rfkwh::Meter &m, JsonObject obj);
// All meters heard since boot, keyed by id "A7,FC,4D"
void getRFKwhMetersJSON(JsonObject root);
