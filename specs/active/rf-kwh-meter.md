# Feature Spec: RF kWh meters (Itho/Klimaatgarant) read by the add-on

Status: Active
Owner: Arjen
Related ADR(s): none (config file in the existing format, no new task)

## Summary

Klimaatgarant heat pump installations have RF kWh meters (RAMSES device type 29, device info "VMS-02J52") for the heat pump, the ventilation unit and the solar panels. Verified on site: a meter answers `RQ 2401` from any sender within tens of ms with `RP 2401`, 13 bytes: `00` plus three big-endian uint32 Wh counters (it also pushes the same to its bound WPU about once an hour). `RQ 2400` answers the counter format (`00 20 20 20`), `RQ 10E0` the device info.

The add-on polls the meters the user configured. The feature is switched on with "kWh meter polling" on the RF settings page (`systemConfig.itho_rf_kwh`); the meters (id, name) and the poll interval live in `/kwhmeters.json` (`KwhMeterConfig`), edited on the "kWh meters" page that appears in the menu when the feature is on. Readings go to REST, MQTT and HA discovery. Meters that are heard but not configured (neighbours) are shown on the page so the user can add them, and nowhere else.

## Acceptance Criteria

- [x] Ids are accepted as hex bytes (`77,FC,4D`, also `:` or no separator) and in ramses notation (`29:261197`); stored and shown as hex bytes
- [x] Up to 8 meters with a name each; interval 10 to 3600 s; the add-on sends one `RQ 2401` per meter per interval, round robin, as itself (own id), one frame (no repeats: the reply would be lost)
- [x] `RP 2401` of 13 bytes from a type 29 device is decoded; a device of another type counts only when its id is configured (the large inverter meter answers as type 37); other 2401 traffic (WPU, fans) is ignored
- [x] `GET /api/v2/kwhmeters`: `enabled`, `interval`, `kwhmeters` keyed by id with `id`, `name`, `e1_kwh`..`e3_kwh`, `lastseen`, `frames`, `configured`; configured meters without a reply yet are listed with zero counters
- [x] MQTT: retained `<base>/kwhmeter/77_FC_4D` with the same fields after every reply, configured meters only
- [x] HA discovery: energy sensors (`total_increasing`, kWh) per configured meter that answered, named after the meter name (fallback "kWh meter <id>"); a counter that is still zero gets none; the list is republished when the configuration changes
- [x] Web UI: websocket `{"kwhsettings":true}` returns the settings plus heard meters; `{"kwhsettings":{interval, meters:[{id,name}]}}` replaces the list and saves
- [x] One system log line per newly heard meter per boot, nothing per poll (flash log budget)
- [ ] Feature off: no polling, no MQTT, no discovery entities (readings of heard meters still appear on the page once switched on)

## Architecture Impact

- New config file `/kwhmeters.json` (class `KwhMeterConfig`, same load/save helpers as the other config files, `saveKwhMeterConfigflag` in the config task)
- New system setting `itho_rf_kwh`
- Poll from the system control loop next to the RF status poll (`pollRFKwhMeters()`), send through `IthoCC1101::sendRQ` (generic RQ, single frame)
- RF receive dispatch (`KWHMETER` packet type), MQTT topic `kwhmeter/<id>`, REST `GET /api/v2/kwhmeters`, HA discovery components, web UI page `kwhmeters`
- Debug action `rfrq` (`src`, `dest`, `opcode`, `data`, `repeats`) sends any RQ from the debug API

## Risks

- A neighbour's meter answers too when its id is entered: the user picks ids from the sticker or from the heard list
- Counter semantics of the three-counter heat pump meter are not confirmed (E1, E2, E3 kept neutral)
- Flash: one config file write per save on the page; polling writes nothing

## Test Requirements

- [x] Native unit test `test_native_rf_kwh`: decode, id parsing (hex and ramses notation), id helpers, store
- [x] Live: on site 2026-11-24 with three meters, 30 s interval, REST output checked; the Klimaatgarant portal comparison is the user's
- [ ] HA integration: entities from the device discovery and from the REST endpoint (follow-up in the integration repo)

## Rollback Strategy

`git revert`; `/kwhmeters.json` and the retained MQTT topics are left behind and harmless.

## Related Components

`main/cc1101/rf_kwh_meter.h`, `main/cc1101/IthoCC1101.{h,cpp}` (`sendRQ`), `main/config/KwhMeterConfig.{h,cpp}`, `main/config/Config.{h,cpp}`, `main/config/SystemConfig.{h,cpp}`, `main/ithodevice/RFKwhMeters.{h,cpp}`, `main/tasks/task_syscontrol.cpp`, `main/tasks/task_configandlog.{h,cpp}`, `main/tasks/task_mqtt.cpp`, `main/HADiscovery.cpp`, `main/websocket.cpp`, `main/api/WebAPIv2.cpp` (`rfrq`), `main/api/WebAPIv2Rest.cpp`, `main/webroot_source/controls_js_sources/html_kwhmeters.html`, `docs/openapi.json`, `test/test_native_rf_kwh/`
