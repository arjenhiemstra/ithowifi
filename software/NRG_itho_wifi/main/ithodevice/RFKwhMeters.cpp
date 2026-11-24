#include "ithodevice/RFKwhMeters.h"
#include "ithodevice/IthoDevice.h"
#include "config/KwhMeterConfig.h"
#include "managers/RFManager.h"
#include "tasks/task_cc1101.h"
#include "globals.h"
#include "sys_log.h"

#include <time.h>

extern bool updateMQTTKwhMeters;
extern bool sendHomeAssistantDiscovery;

rfkwh::Store rfKwhMeters;

void parseRFKwhMeter(const uint8_t *payload, uint8_t len, uint8_t srcId0, uint8_t srcId1, uint8_t srcId2)
{
  if (systemConfig.itho_rf_kwh != 1)
    return;
  rfkwh::Reading r;
  if (!rfkwh::decode2401(payload, len, r))
    return;

  time_t now;
  time(&now);
  const uint8_t id[3] = {srcId0, srcId1, srcId2};

  if (xSemaphoreTake(rfStatusMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    return; // a reader is busy, the next poll comes soon enough
  bool isNew = false;
  const int slot = rfKwhMeters.update(id, r, static_cast<uint32_t>(now), isNew);
  xSemaphoreGive(rfStatusMutex);

  if (slot < 0)
    return; // table full: stay silent, the frame repeats
  if (isNew)
  {
    char rid[12];
    rfkwh::formatId(id, rid, sizeof(rid));
    N_LOG("RF: kWh meter %s found, %lu Wh", rid, (unsigned long)r.wh[0]); // once per meter per boot
    if (systemConfig.mqtt_ha_active && kwhMeterConfig.find(id) >= 0)
      sendHomeAssistantDiscovery = true; // counters now known, so the energy sensors can be added
  }
  if (kwhMeterConfig.find(id) >= 0)
    updateMQTTKwhMeters = true;
}

// One meter per call, spaced so that each configured meter is asked once per interval. The reply
// (RP 2401) arrives within tens of ms and is handled by the normal receive path.
bool pollRFKwhMeters()
{
  static uint32_t lastPoll = 0;
  static int next = 0;
  if (systemConfig.itho_rf_kwh != 1 || !systemConfig.rfInitOK)
    return false;
  const int n = kwhMeterConfig.count();
  if (n == 0)
    return false;
  const uint32_t spacing = (kwhMeterConfig.interval * 1000UL) / n;
  if (lastPoll != 0 && millis() - lastPoll < spacing)
    return false;
  lastPoll = millis();

  // next configured slot, round robin
  for (int tries = 0; tries < rfkwh::MAX_METERS; tries++)
  {
    const int i = (next + tries) % rfkwh::MAX_METERS;
    if (!kwhMeterConfig.meters[i].used)
      continue;
    next = (i + 1) % rfkwh::MAX_METERS;
    const uint8_t command[] = {0x24, 0x01, 0x01, 0x00};
    disableRF_ISR();
    rfManager.radio.sendRQ(nullptr, kwhMeterConfig.meters[i].id, command);
    enableRF_ISR();
    return true;
  }
  return false;
}

void getRFKwhMeterJSON(const rfkwh::Meter &m, JsonObject obj)
{
  char rid[12];
  rfkwh::formatId(m.id, rid, sizeof(rid));
  obj["id"] = rid;
  obj["name"] = kwhMeterConfig.nameOf(m.id);
  obj["e1_kwh"] = m.reading.wh[0] / 1000.0;
  obj["e2_kwh"] = m.reading.wh[1] / 1000.0;
  obj["e3_kwh"] = m.reading.wh[2] / 1000.0;
  obj["lastseen"] = m.lastSeen;
  obj["frames"] = m.frames;
  obj["configured"] = kwhMeterConfig.find(m.id) >= 0;
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
  // configured but not heard yet: show them with empty counters
  for (const KwhMeterConfig::Meter &c : kwhMeterConfig.meters)
  {
    if (!c.used || rfKwhMeters.find(c.id) >= 0)
      continue;
    rfkwh::Meter m;
    memcpy(m.id, c.id, 3);
    char rid[12];
    rfkwh::formatId(c.id, rid, sizeof(rid));
    getRFKwhMeterJSON(m, root[rid].to<JsonObject>());
  }
  xSemaphoreGive(rfStatusMutex);
}

void getRFKwhSettingsJSON(JsonObject root)
{
  root["enabled"] = systemConfig.itho_rf_kwh;
  root["rfInitOK"] = systemConfig.rfInitOK;
  root["max"] = rfkwh::MAX_METERS;
  kwhMeterConfig.get(root);
  JsonArray heard = root["heard"].to<JsonArray>();
  if (xSemaphoreTake(rfStatusMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    return;
  for (int i = 0; i < rfkwh::MAX_METERS; i++)
  {
    const rfkwh::Meter &m = rfKwhMeters.at(i);
    if (!m.used)
      continue;
    getRFKwhMeterJSON(m, heard.add<JsonObject>());
  }
  xSemaphoreGive(rfStatusMutex);
}
