#include "ithodevice/RFKwhMeters.h"
#include "ithodevice/IthoDevice.h"
#include "globals.h"
#include "sys_log.h"

#include <time.h>

extern bool updateMQTTKwhMeters;
extern bool sendHomeAssistantDiscovery;

rfkwh::Store rfKwhMeters;

void parseRFKwhMeter(const uint8_t *payload, uint8_t len, uint8_t srcId0, uint8_t srcId1, uint8_t srcId2)
{
  rfkwh::Reading r;
  if (!rfkwh::decode2401(payload, len, r))
    return;

  time_t now;
  time(&now);
  const uint8_t id[3] = {srcId0, srcId1, srcId2};

  if (xSemaphoreTake(rfStatusMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    return; // a reader is busy, the meter repeats and pushes again within the hour
  bool isNew = false;
  const int slot = rfKwhMeters.update(id, r, static_cast<uint32_t>(now), isNew);
  xSemaphoreGive(rfStatusMutex);

  if (slot < 0)
    return; // table full: stay silent, a log line here would repeat every hour
  if (isNew)
  {
    char rid[12];
    rfkwh::formatId(id, rid, sizeof(rid));
    N_LOG("RF: kWh meter %s found, %lu Wh", rid, (unsigned long)r.wh[0]); // once per meter per boot
    if (systemConfig.mqtt_ha_active)
      sendHomeAssistantDiscovery = true; // new energy sensors
  }
  updateMQTTKwhMeters = true;
}

void getRFKwhMeterJSON(const rfkwh::Meter &m, JsonObject obj)
{
  char rid[12];
  rfkwh::formatId(m.id, rid, sizeof(rid));
  obj["id"] = rid;
  obj["e1_kwh"] = m.reading.wh[0] / 1000.0;
  obj["e2_kwh"] = m.reading.wh[1] / 1000.0;
  obj["e3_kwh"] = m.reading.wh[2] / 1000.0;
  obj["lastseen"] = m.lastSeen;
  obj["frames"] = m.frames;
}

void getRFKwhMetersJSON(JsonObject root)
{
  if (xSemaphoreTake(rfStatusMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    return;
  for (int i = 0; i < rfkwh::MAX_METERS; i++)
  {
    const rfkwh::Meter &m = rfKwhMeters.at(i);
    if (!m.used)
      continue;
    char rid[12];
    rfkwh::formatId(m.id, rid, sizeof(rid));
    getRFKwhMeterJSON(m, root[rid].to<JsonObject>());
  }
  xSemaphoreGive(rfStatusMutex);
}
