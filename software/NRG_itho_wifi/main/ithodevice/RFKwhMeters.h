#pragma once
// RF kWh meters (Itho/Klimaatgarant, RAMSES type 29): readings store, RF callback, poller and JSON.
// The feature is on when systemConfig.itho_rf_kwh == 1; the meters to poll and their names live in
// kwhMeterConfig. Readers take rfStatusMutex, the RF task writes under it.

#include <ArduinoJson.h>
#include "cc1101/rf_kwh_meter.h"

extern rfkwh::Store rfKwhMeters;

// Callback for IthoCC1101::setRFKwhCallback: decodes RP 2401 (push or reply) and updates the store
void parseRFKwhMeter(const uint8_t *payload, uint8_t len, uint8_t srcId0, uint8_t srcId1, uint8_t srcId2);

// Called from the system control loop: sends RQ 2401 to the configured meters, round robin, so every
// meter is asked once per kwhMeterConfig.interval. Returns true when a request was sent.
bool pollRFKwhMeters();

// One meter: id, name, e1_kwh..e3_kwh, lastseen, frames, configured
void getRFKwhMeterJSON(const rfkwh::Meter &m, JsonObject obj);
// All meters heard since boot plus the configured ones not heard yet, keyed by id "77,FC,4D"
void getRFKwhMetersJSON(JsonObject root);
// Settings page: enabled, interval, meters[] (id, name) and heard[] (meters seen but not configured)
void getRFKwhSettingsJSON(JsonObject root);
