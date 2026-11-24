#pragma once
// RF kWh meters the add-on polls (RQ 2401 over RF): the list of meter ids with a name each and the poll
// interval. Stored in /kwhmeters.json. The feature itself is switched on with systemConfig.itho_rf_kwh.

#define KWH_METER_CONFIG_VERSION "001"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "cc1101/rf_kwh_meter.h"

class KwhMeterConfig
{
public:
  static constexpr uint16_t INTERVAL_MIN = 10;   // seconds between two polls of the same meter
  static constexpr uint16_t INTERVAL_MAX = 3600;
  static constexpr size_t NAME_LEN = 32;

  struct Meter
  {
    uint8_t id[3]{};
    char name[NAME_LEN]{};
    bool used{false};
  };

  uint16_t interval; // seconds
  Meter meters[rfkwh::MAX_METERS];
  char config_struct_version[4];

  mutable bool configLoaded;

  KwhMeterConfig();

  bool set(JsonObject);
  void get(JsonObject) const;

  int count() const;
  int find(const uint8_t id[3]) const;      // slot or -1
  const char *nameOf(const uint8_t id[3]) const; // "" when not configured

  // see rfkwh::parseId
  static bool parseId(const char *s, uint8_t out[3]);
};

extern KwhMeterConfig kwhMeterConfig;
