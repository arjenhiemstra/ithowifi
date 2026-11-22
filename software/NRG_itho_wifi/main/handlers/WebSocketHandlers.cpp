#include "tasks/task_web.h"
#include "managers/ServicetoolBridge.h"

void wifiStat()
{
  JsonDocument root = wifiInfoJson();

  notifyClients(root.as<JsonObject>());
}
void jsonSystemstat()
{
  JsonDocument root;
  JsonObject systemstat = root["systemstat"].to<JsonObject>();
  systemstat["freemem"] = sys.getMemHigh();
  systemstat["memlow"] = sys.getMemLow();
  systemstat["mqqtstatus"] = MQTT_conn_state;
  systemstat["itho"] = ithoCurrentVal;
  systemstat["itho_low"] = systemConfig.itho_low;
  systemstat["itho_medium"] = systemConfig.itho_medium;
  systemstat["itho_high"] = systemConfig.itho_high;

  if (SHT3x_original || SHT3x_alternative || itho_internal_hum_temp)
  {
    systemstat["sensor_temp"] = ithoTemp;
    systemstat["sensor_hum"] = ithoHum;
    systemstat["sensor"] = 1;
  }
  systemstat["itho_llm"] = remotes.getllModeTime();
  systemstat["copy_id"] = virtualRemotes.getllModeTime();
  systemstat["ithoinit"] = ithoInitResult;
  systemstat["fan_demand"] = ithoFanDemand;
  systemstat["ota_progress"] = otaUpdateProgress;
  systemstat["uuid"] = uuid;
  // servicetool session state, used by the web UI for the session banner
  servicetoolGetStatusJSON(systemstat["servicetool"].to<JsonObject>(), false);

  notifyClients(root.as<JsonObject>());
}
