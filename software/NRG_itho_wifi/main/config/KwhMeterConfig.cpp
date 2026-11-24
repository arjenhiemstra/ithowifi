#include "config/KwhMeterConfig.h"

#include <cstring>

KwhMeterConfig kwhMeterConfig;

KwhMeterConfig::KwhMeterConfig()
{
  interval = 60;
  strlcpy(config_struct_version, KWH_METER_CONFIG_VERSION, sizeof(config_struct_version));
  configLoaded = false;
}

int KwhMeterConfig::count() const
{
  int n = 0;
  for (const Meter &m : meters)
    n += m.used ? 1 : 0;
  return n;
}

int KwhMeterConfig::find(const uint8_t id[3]) const
{
  for (int i = 0; i < rfkwh::MAX_METERS; i++)
    if (meters[i].used && memcmp(meters[i].id, id, 3) == 0)
      return i;
  return -1;
}

const char *KwhMeterConfig::nameOf(const uint8_t id[3]) const
{
  const int i = find(id);
  return i < 0 ? "" : meters[i].name;
}

bool KwhMeterConfig::parseId(const char *s, uint8_t out[3])
{
  return rfkwh::parseId(s, out);
}

// {"interval":60,"meters":[{"id":"77,FC,4D","name":"Heat pump"}, ...]}; a meters array replaces the whole list
bool KwhMeterConfig::set(JsonObject obj)
{
  bool updated = false;
  if (!obj["interval"].isNull())
  {
    int v = obj["interval"].as<int>();
    if (v < INTERVAL_MIN)
      v = INTERVAL_MIN;
    if (v > INTERVAL_MAX)
      v = INTERVAL_MAX;
    interval = static_cast<uint16_t>(v);
    updated = true;
  }
  if (obj["meters"].is<JsonArray>())
  {
    Meter fresh[rfkwh::MAX_METERS];
    int n = 0;
    for (JsonObject m : obj["meters"].as<JsonArray>())
    {
      uint8_t id[3];
      if (n >= rfkwh::MAX_METERS || !parseId(m["id"] | "", id))
        continue;
      bool dup = false;
      for (int i = 0; i < n; i++)
        dup |= memcmp(fresh[i].id, id, 3) == 0;
      if (dup)
        continue;
      memcpy(fresh[n].id, id, 3);
      strlcpy(fresh[n].name, m["name"] | "", sizeof(fresh[n].name));
      fresh[n].used = true;
      n++;
    }
    for (int i = 0; i < rfkwh::MAX_METERS; i++)
      meters[i] = fresh[i];
    updated = true;
  }
  return updated;
}

void KwhMeterConfig::get(JsonObject obj) const
{
  obj["interval"] = interval;
  JsonArray arr = obj["meters"].to<JsonArray>();
  for (const Meter &m : meters)
  {
    if (!m.used)
      continue;
    char rid[12];
    rfkwh::formatId(m.id, rid, sizeof(rid));
    JsonObject o = arr.add<JsonObject>();
    o["id"] = rid;
    o["name"] = m.name;
  }
}
