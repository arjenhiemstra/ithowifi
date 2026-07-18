# Glossary (domain terms)

| Term | Meaning |
|---|---|
| Itho unit | The central ventilation/heat-pump appliance being controlled (not the ESP32 add-on) |
| CVE add-on | I2C add-on module for CVE-series Itho units |
| non-CVE add-on | I2C add-on module for non-CVE Itho units (HRU, DemandFlow, WPU, AutoTemp) |
| RF standalone mode | Controlling/monitoring an Itho unit purely over CC1101 RF, no I2C wiring (e.g. HRU 400) |
| Virtual remote | An RF remote *emulated in firmware* and sent over I2C to the Itho unit, as opposed to a physical RF remote |
| Physical RF remote | A real Itho RF remote registered/bound to the add-on's CC1101 radio |
| HA Discovery | Home Assistant MQTT Discovery — auto-registers the device as an HA entity via a discovery config topic |
| Domoticz bridge | MQTT topic pair (`mqtt_domoticzin_topic`/`mqtt_domoticzout_topic`) for Domoticz-compatible integration |
| OTA | Over-the-air firmware update, triggered from the web UI or `/api/v2/ota` |
| Bind | Pairing/registering a physical RF remote to the add-on (see `isBindInitiatorActive` in `IthoCC1101`) |
| Sniffer | Passive I2C bus monitor used for protocol reverse-engineering/debug (`i2c_sniffer.cpp`) |
| Failsafe boot | Boot-time check that can force AP mode / OTA recovery if hardware/config is unhealthy (`task_init.cpp`) |

## Related

[.ai/terminology.md](terminology.md) — protocol/code abbreviations · [.ai/project-overview.md](project-overview.md)
