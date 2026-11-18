# Feature Spec: RF kWh meters (Itho/Klimaatgarant) read by the add-on

Status: Active
Owner: Arjen
Related ADR(s): none (no new task, manager or config format)

## Summary

Klimaatgarant heat pump installations have RF kWh meters (RAMSES device type 29, device info "VMS-02J52") for the heat pump, the ventilation unit and the solar panels. Each meter pushes its counters to the bound WPU about once an hour as `RP 2401`, 13 bytes: `00` plus three big-endian uint32 Wh counters. No request is on air, so an add-on with the RF module can listen for these frames and publish the counters. Target: customers with the add-on on a WPU plus the RF module. First stage (this spec): passive listening, MQTT, HA discovery and REST. Reading on request (RQ 2401 / 2400) is tried on site and gets its own spec when it works.

## Acceptance Criteria

- [ ] An `RP 2401` of 13 bytes from a device whose first id byte has type 29 is decoded into three Wh counters; other 2401 traffic (WPU, fans) is ignored
- [ ] Every meter heard since boot appears in `GET /api/v2/kwhmeters` keyed by its id as hex bytes (`A7,FC,4D`) with `e1_kwh`, `e2_kwh`, `e3_kwh`, `lastseen`, `frames`
- [ ] Each push is published retained on `<base>/kwhmeter/A7_FC_4D` with the same fields
- [ ] With HA discovery on, each meter gets energy sensors (`total_increasing`, kWh); a counter that is still zero gets none
- [ ] A new meter writes one system log line; later pushes write none (flash log budget)
- [ ] Up to 8 meters; a 9th is ignored silently

## Architecture Impact

- Touches: RF receive dispatch (`IthoCC1101` handler + callback, same shape as 31DA/31D9), MQTT topics (new `kwhmeter/<id>`), REST API (`GET /api/v2/kwhmeters`), HA discovery components
- No task chain, manager, config format or hardware abstraction change

## Risks

- Neighbours' meters are heard too (Klimaatgarant installations are mesh-like, 4 WPUs in the capture): all are published, the user picks by id. A whitelist is a follow-up when needed.
- Wrong field semantics: the three counters of the heat pump meter are assumed per phase, not verified. Names stay neutral (E1, E2, E3).
- Flash/RAM: the store is 8 × 32 bytes static; code is small (decode, JSON, discovery loop). No flash writes.

## Test Requirements

- [x] Native unit test `test_native_rf_kwh`: decode of captured frames, bad lengths, id helpers, store (new, repeat, replace, full)
- [ ] Live-hardware: on site with the customer's meters, check the REST output against the Klimaatgarant portal values; `tests/` has no automated coverage (needs a type 29 transmitter)

## Rollback Strategy

Safe to `git revert`: no persisted state, no config migration. Retained MQTT topics stay on the broker until cleared.

## Related Components

`main/cc1101/rf_kwh_meter.h`, `main/cc1101/IthoPacket.h`, `main/cc1101/IthoCC1101.{h,cpp}`, `main/ithodevice/RFKwhMeters.{h,cpp}`, `main/tasks/task_cc1101.cpp`, `main/tasks/task_mqtt.{h,cpp}`, `main/HADiscovery.cpp`, `main/api/WebAPIv2Rest.cpp`, `docs/openapi.json`, `test/test_native_rf_kwh/`
